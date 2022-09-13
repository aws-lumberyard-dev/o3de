/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <AzCore/Console/Console.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CVAR(
            bool,
            r_forceRootShaderVariantUsage,
            false,
            [](const bool&)
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("MeshFeatureProcessor.ForceRebuildDrawPackets");
            },
            ConsoleFunctorFlags::Null,
            "(For Testing) Forces usage of root shader variant in the mesh draw packet level, ignoring any other shader variants that may "
            "exist.");

        MeshDrawPacket::MeshDrawPacket(
            ModelLod& modelLod,
            size_t modelLodMeshIndex,
            Data::Instance<Material> materialOverride,
            Data::Instance<ShaderResourceGroup> objectSrg,
            const MaterialModelUvOverrideMap& materialModelUvMap)
            : m_modelLod(&modelLod)
            , m_modelLodMeshIndex(modelLodMeshIndex)
            , m_objectSrg(objectSrg)
            , m_material(materialOverride)
            , m_materialModelUvMap(materialModelUvMap)
        {
            if (!m_material)
            {
                const ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];
                m_material = mesh.m_material;
            }
        }

        Data::Instance<Material> MeshDrawPacket::GetMaterial() const
        {
            return m_material;
        }

        bool MeshDrawPacket::SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value)
        {
            // check if the material owns this option in any of its shaders, if so it can't be set externally
            for (auto& shaderItem : m_material->GetShaderCollection())
            {
                const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                if (index.IsValid())
                {
                    if (shaderItem.MaterialOwnsShaderOption(index))
                    {
                        return false;
                    }
                }
            }

            for (auto& shaderItem : m_material->GetShaderCollection())
            {
                const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                if (index.IsValid())
                {
                    // try to find an existing option entry in the list
                    auto itEntry = AZStd::find_if(
                        m_shaderOptions.begin(),
                        m_shaderOptions.end(),
                        [&shaderOptionName](const ShaderOptionPair& entry)
                        {
                            return entry.first == shaderOptionName;
                        });

                    // store the option name and value, they will be used in DoUpdate() to select the appropriate shader variant
                    if (itEntry == m_shaderOptions.end())
                    {
                        m_shaderOptions.push_back({ shaderOptionName, value });
                    }
                    else
                    {
                        itEntry->second = value;
                    }
                }
            }

            return true;
        }

        bool MeshDrawPacket::Update(const Scene& parentScene, bool forceUpdate /*= false*/)
        {
            // Why we need to check "!m_material->NeedsCompile()"...
            //    Frame A:
            //      - Material::SetPropertyValue("foo",...). This bumps the material's CurrentChangeId()
            //      - Material::Compile() updates all the material's outputs (SRG data, shader selection, shader options, etc).
            //      - Material::SetPropertyValue("bar",...). This bumps the materials' CurrentChangeId() again.
            //      - We do not process Material::Compile() a second time because you can only call SRG::Compile() once per frame.
            //      Material::Compile()
            //        will be processed on the next frame. (See implementation of Material::Compile())
            //      - MeshDrawPacket::Update() is called. It runs DoUpdate() to rebuild the draw packet, but everything is still in the
            //      state when "foo" was
            //        set. The "bar" changes haven't been applied yet. It also sets m_materialChangeId to GetCurrentChangeId(), which
            //        corresponds to "bar" not "foo".
            //    Frame B:
            //      - Something calls Material::Compile(). This finally updates the material's outputs with the latest data corresponding to
            //      "bar".
            //      - MeshDrawPacket::Update() is called. But since the GetCurrentChangeId() hasn't changed since last time, DoUpdate() is
            //      not called.
            //      - The mesh continues rendering with only the "foo" change applied, indefinitely.

            if (forceUpdate || (!m_material->NeedsCompile() && m_materialChangeId != m_material->GetCurrentChangeId()))
            {
                DoUpdate(parentScene);
                m_materialChangeId = m_material->GetCurrentChangeId();
                return true;
            }

            return false;
        }

        bool MeshDrawPacket::NeedsUpdate(bool forceUpdate /*= false*/)
        {
            if (forceUpdate || (!m_material->NeedsCompile() && m_materialChangeId != m_material->GetCurrentChangeId()))
            {
                return true;
            }
            return false;
        }

        bool MeshDrawPacket::DoUpdate(const Scene& parentScene)
        {
            const ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];

            if (!m_material)
            {
                AZ_Warning("MeshDrawPacket", false, "No material provided for mesh. Skipping.");
                return false;
            }

            ShaderReloadDebugTracker::ScopedSection reloadSection("MeshDrawPacket::DoUpdate");

            RHI::DrawPacketBuilder drawPacketBuilder;
            drawPacketBuilder.Begin(nullptr);

            drawPacketBuilder.SetDrawArguments(mesh.m_drawArguments);
            drawPacketBuilder.SetIndexBufferView(mesh.m_indexBufferView);
            drawPacketBuilder.AddShaderResourceGroup(m_objectSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddShaderResourceGroup(m_material->GetRHIShaderResourceGroup());

            // We build the list of used shaders in a local list rather than m_activeShaders so that
            // if DoUpdate() fails it won't modify any member data.
            MeshDrawPacket::ShaderList shaderList;
            shaderList.reserve(m_activeShaders.size());

            // We have to keep a list of these outside the loops that collect all the shaders because the DrawPacketBuilder
            // keeps pointers to StreamBufferViews until DrawPacketBuilder::End() is called. And we use a fixed_vector to guarantee
            // that the memory won't be relocated when new entries are added.
            AZStd::fixed_vector<ModelLod::StreamBufferViewList, RHI::DrawPacketBuilder::DrawItemCountMax> streamBufferViewsPerShader;

            m_perDrawSrgs.clear();

            auto appendShader = [&](const ShaderCollection::Item& shaderItem)
            {
                // Skip the shader item without creating the shader instance
                // if the mesh is not going to be rendered based on the draw tag
                RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
                RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

                // Use the explicit draw list override if exists.
                RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                if (drawListTag.IsNull())
                {
                    Data::Asset<RPI::ShaderAsset> shaderAsset = shaderItem.GetShaderAsset();
                    if (!shaderAsset.IsReady())
                    {
                        // The shader asset needs to be loaded before we can check the draw tag.
                        // If it's not loaded yet, the instance database will do a blocking load
                        // when we create the instance below, so might as well load it now.
                        shaderAsset.QueueLoad();

                        if (shaderAsset.IsLoading())
                        {
                            shaderAsset.BlockUntilLoadComplete();
                        }
                    }

                    drawListTag = drawListTagRegistry->FindTag(shaderAsset->GetDrawListName());
                }

                if (!parentScene.HasOutputForPipelineState(drawListTag))
                {
                    // drawListTag not found in this scene, so don't render this item
                    return false;
                }

                Data::Instance<Shader> shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                if (!shader)
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        false,
                        "Shader '%s'. Failed to find or create instance",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
                // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify
                // those options) [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default
                // values for us, and remove this step here.
                RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
                shaderOptions.SetUnspecifiedToDefaultValues();

                // [GFX_TODO][ATOM-14476]: according to this usage, we should make the shader input contract uniform across all shader
                // variants.
                m_modelLod->CheckOptionalStreams(
                    shaderOptions,
                    shader->GetInputContract(),
                    m_modelLodMeshIndex,
                    m_materialModelUvMap,
                    m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());

                // apply shader options from this draw packet to the ShaderItem
                for (auto& meshShaderOption : m_shaderOptions)
                {
                    Name& name = meshShaderOption.first;
                    RPI::ShaderOptionValue& value = meshShaderOption.second;

                    ShaderOptionIndex index = shaderOptions.FindShaderOptionIndex(name);
                    if (index.IsValid())
                    {
                        shaderOptions.SetValue(name, value);
                    }
                }

                const ShaderVariantId requestedVariantId = shaderOptions.GetShaderVariantId();
                const ShaderVariant& variant =
                    r_forceRootShaderVariantUsage ? shader->GetRootVariant() : shader->GetVariant(requestedVariantId);

                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                variant.ConfigurePipelineState(pipelineStateDescriptor);

                // Render states need to merge the runtime variation.
                // This allows materials to customize the render states that the shader uses.
                const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
                RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptor.m_renderStates);

                auto& streamBufferViews = streamBufferViewsPerShader.emplace_back();

                UvStreamTangentBitmask uvStreamTangentBitmask;

                if (!m_modelLod->GetStreamsForMesh(
                        pipelineStateDescriptor.m_inputStreamLayout,
                        streamBufferViews,
                        &uvStreamTangentBitmask,
                        shader->GetInputContract(),
                        m_modelLodMeshIndex,
                        m_materialModelUvMap,
                        m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
                {
                    return false;
                }

                auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout(shader->GetSupervariantIndex());
                Data::Instance<ShaderResourceGroup> drawSrg;
                if (drawSrgLayout)
                {
                    // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being null
                    drawSrg =
                        RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawSrgLayout->GetName());

                    if (!variant.IsFullyBaked() && drawSrgLayout->HasShaderVariantKeyFallbackEntry())
                    {
                        drawSrg->SetShaderVariantKeyFallbackValue(shaderOptions.GetShaderVariantKeyFallbackValue());
                    }

                    // Pass UvStreamTangentBitmask to the shader if the draw SRG has it.
                    {
                        AZ::Name shaderUvStreamTangentBitmask = AZ::Name(UvStreamTangentBitmask::SrgName);
                        auto index = drawSrg->FindShaderInputConstantIndex(shaderUvStreamTangentBitmask);

                        if (index.IsValid())
                        {
                            drawSrg->SetConstant(index, uvStreamTangentBitmask.GetFullTangentBitmask());
                        }
                    }

                    drawSrg->Compile();
                }

                parentScene.ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!pipelineState)
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        false,
                        "Shader '%s'. Failed to acquire default pipeline state",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = drawListTag;
                drawRequest.m_pipelineState = pipelineState;
                drawRequest.m_streamBufferViews = streamBufferViews;
                drawRequest.m_stencilRef = m_stencilRef;
                drawRequest.m_sortKey = m_sortKey;
                if (drawSrg)
                {
                    drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                    m_perDrawSrgs.push_back(drawSrg);
                }
                drawPacketBuilder.AddDrawItem(drawRequest);

                ShaderData shaderData;
                shaderData.m_shader = AZStd::move(shader);
                shaderData.m_requestedShaderVariantId = requestedVariantId;
                shaderData.m_activeShaderVariantId = variant.GetShaderVariantId();
                shaderList.emplace_back(AZStd::move(shaderData));

                return true;
            };

            m_material->ApplyGlobalShaderOptions();

            for (auto& shaderItem : m_material->GetShaderCollection())
            {
                if (shaderItem.IsEnabled())
                {
                    if (shaderList.size() == RHI::DrawPacketBuilder::DrawItemCountMax)
                    {
                        AZ_Error(
                            "MeshDrawPacket",
                            false,
                            "Material has more than the limit of %d active shader items.",
                            RHI::DrawPacketBuilder::DrawItemCountMax);
                        return false;
                    }

                    appendShader(shaderItem);
                }
            }

            m_drawPacket = drawPacketBuilder.End();

            if (m_drawPacket)
            {
                m_activeShaders = shaderList;
                m_materialSrg = m_material->GetRHIShaderResourceGroup();
                return true;
            }
            else
            {
                return false;
            }
        }

        
        void MeshDrawPacket::SetBatchUpdateInput(PrepareMeshDrawPacketUpdateBatchInput& input)
        {
            input.m_drawPacket = this;
        }

        
        void AppendMeshDrawPacketShaderItemBatch(
            const Scene& parentScene,
            AZStd::span<PrepareMeshDrawPacketUpdateBatchInput> prepareUpdateBatchInput,
            AZStd::span<AppendMeshDrawPacketShaderBatchInput> batchInput,
            AZStd::span<AppendMeshDrawPacketShaderBatchOutput> batchOutput)
        {
            AZ_PROFILE_SCOPE(RPI, "AppendMeshDrawPacketShaderItemBatch");
            for (size_t batchInputIndex = 0; batchInputIndex < batchInput.size(); ++batchInputIndex)
            {
                AppendMeshDrawPacketShaderBatchInput& input = batchInput[batchInputIndex];
                AppendMeshDrawPacketShaderBatchOutput& output = batchOutput[batchInputIndex];
                size_t prepareUpdateInputIndex = batchInput[batchInputIndex].m_prepareUpdateBatchIndex;
                const ShaderCollection::Item& shaderItem = *input.shaderItem;
                Data::Instance<ModelLod> m_modelLod = input.m_modelLod;
                output.shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                Data::Instance<Shader> shader = output.shader;
                AZStd::fixed_vector<ModelLod::StreamBufferViewList, RHI::DrawPacketBuilder::DrawItemCountMax>& streamBufferViewsPerShader =
                    batchOutput[batchInputIndex].streamBufferViewsPerShader;
                size_t& m_modelLodMeshIndex = prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_modelLodMeshIndex;
                Data::Instance<Material> m_material = prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_material;
                MaterialModelUvOverrideMap& m_materialModelUvMap =
                    prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_materialModelUvMap;
                MeshDrawPacket::ShaderOptionVector& m_shaderOptions =
                    prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_shaderOptions;
                AZStd::fixed_vector<Data::Instance<ShaderResourceGroup>, RHI::DrawPacketBuilder::DrawItemCountMax>& m_perDrawSrgs =
                    prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_perDrawSrgs;

                RHI::DrawListTag drawListTag = batchInput[batchInputIndex].m_drawListTag;

                // Set the sort key for the draw packet
                RHI::DrawItemSortKey m_sortKey = prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_sortKey;

                // Set the stencil value for this draw packet
                uint8_t m_stencilRef = prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacket->m_stencilRef;
                RHI::DrawPacketBuilder& drawPacketBuilder = prepareUpdateBatchInput[prepareUpdateInputIndex].m_drawPacketBuilder;
                MeshDrawPacket::ShaderList& shaderList = prepareUpdateBatchInput[prepareUpdateInputIndex].shaderList;

                if (!shader)
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        false,
                        "Shader '%s'. Failed to find or create instance",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());
                    // TODO: handle failure of an individual without failing the whole batch
                    return;
                }

                // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
                // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify
                // those options) [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default
                // values for us, and remove this step here.
                RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
                shaderOptions.SetUnspecifiedToDefaultValues();

                // [GFX_TODO][ATOM-14476]: according to this usage, we should make the shader input contract uniform across all shader
                // variants.
                m_modelLod->CheckOptionalStreams(
                    shaderOptions,
                    shader->GetInputContract(),
                    m_modelLodMeshIndex,
                    m_materialModelUvMap,
                    m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());

                // apply shader options from this draw packet to the ShaderItem
                for (auto& meshShaderOption : m_shaderOptions)
                {
                    Name& name = meshShaderOption.first;
                    RPI::ShaderOptionValue& value = meshShaderOption.second;

                    ShaderOptionIndex index = shaderOptions.FindShaderOptionIndex(name);
                    if (index.IsValid())
                    {
                        shaderOptions.SetValue(name, value);
                    }
                }

                const ShaderVariantId requestedVariantId = shaderOptions.GetShaderVariantId();
                const ShaderVariant& variant =
                    r_forceRootShaderVariantUsage ? shader->GetRootVariant() : shader->GetVariant(requestedVariantId);

                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                variant.ConfigurePipelineState(pipelineStateDescriptor);

                // Render states need to merge the runtime variation.
                // This allows materials to customize the render states that the shader uses.
                const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
                RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptor.m_renderStates);

                auto& streamBufferViews = streamBufferViewsPerShader.emplace_back();

                UvStreamTangentBitmask uvStreamTangentBitmask;

                if (!m_modelLod->GetStreamsForMesh(
                        pipelineStateDescriptor.m_inputStreamLayout,
                        streamBufferViews,
                        &uvStreamTangentBitmask,
                        shader->GetInputContract(),
                        m_modelLodMeshIndex,
                        m_materialModelUvMap,
                        m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
                {
                    // TODO: handle failure of an individual without failing the whole batch
                    return;
                }

                auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout(shader->GetSupervariantIndex());
                Data::Instance<ShaderResourceGroup> drawSrg;
                if (drawSrgLayout)
                {
                    // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being
                    // null
                    drawSrg =
                        RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawSrgLayout->GetName());

                    if (!variant.IsFullyBaked() && drawSrgLayout->HasShaderVariantKeyFallbackEntry())
                    {
                        drawSrg->SetShaderVariantKeyFallbackValue(shaderOptions.GetShaderVariantKeyFallbackValue());
                    }

                    // Pass UvStreamTangentBitmask to the shader if the draw SRG has it.
                    {
                        AZ::Name shaderUvStreamTangentBitmask = AZ::Name(UvStreamTangentBitmask::SrgName);
                        auto index = drawSrg->FindShaderInputConstantIndex(shaderUvStreamTangentBitmask);

                        if (index.IsValid())
                        {
                            drawSrg->SetConstant(index, uvStreamTangentBitmask.GetFullTangentBitmask());
                        }
                    }

                    drawSrg->Compile();
                }

                parentScene.ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!pipelineState)
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        false,
                        "Shader '%s'. Failed to acquire default pipeline state",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());

                    // TODO: handle failure of an individual without failing the whole batch
                    return;
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = drawListTag;
                drawRequest.m_pipelineState = pipelineState;
                drawRequest.m_streamBufferViews = streamBufferViews;
                drawRequest.m_stencilRef = m_stencilRef;
                drawRequest.m_sortKey = m_sortKey;
                if (drawSrg)
                {
                    drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                    m_perDrawSrgs.push_back(drawSrg);
                }
                drawPacketBuilder.AddDrawItem(drawRequest);

                MeshDrawPacket::ShaderData shaderData;
                shaderData.m_shader = AZStd::move(shader);
                shaderData.m_requestedShaderVariantId = requestedVariantId;
                shaderData.m_activeShaderVariantId = variant.GetShaderVariantId();
                shaderList.emplace_back(AZStd::move(shaderData));

                // TODO: handle failure of an individual without failing the whole batch
                return;
            }
        }

        void PrepareMeshDrawPacketUpdateBatch(
            const Scene& parentScene,
            AZStd::span<PrepareMeshDrawPacketUpdateBatchInput> batchInput)
        {
            AZ_PROFILE_SCOPE(RPI, "PrepareMeshDrawPacketUpdateBatch");
            size_t activeShaderItemCount = 0;
            AZStd::vector<AZStd::bitset<RHI::DrawPacketBuilder::DrawItemCountMax>> activeShaderItems(batchInput.size());
            ShaderReloadDebugTracker::ScopedSection reloadSection("PrepareMeshDrawPacketUpdateBatch");

            for (size_t inputIndex = 0; inputIndex < batchInput.size(); ++inputIndex)
            {
                PrepareMeshDrawPacketUpdateBatchInput& input = batchInput[inputIndex];
                const ModelLod::Mesh& mesh = input.m_drawPacket->m_modelLod->GetMeshes()[input.m_drawPacket->m_modelLodMeshIndex];

                if (!input.m_drawPacket->m_material)
                {
                    AZ_Warning("MeshDrawPacket", false, "No material provided for mesh. Skipping.");
                    // TODO: handle failure of an individual without failing the whole batch
                    return;
                }


                RHI::DrawPacketBuilder& drawPacketBuilder = input.m_drawPacketBuilder;
                drawPacketBuilder.Begin(nullptr);

                drawPacketBuilder.SetDrawArguments(mesh.m_drawArguments);
                drawPacketBuilder.SetIndexBufferView(mesh.m_indexBufferView);
                drawPacketBuilder.AddShaderResourceGroup(input.m_drawPacket->m_objectSrg->GetRHIShaderResourceGroup());
                drawPacketBuilder.AddShaderResourceGroup(input.m_drawPacket->m_material->GetRHIShaderResourceGroup());

                // Count the drawItems we need
                input.m_drawPacket->m_material->ApplyGlobalShaderOptions();

                ShaderCollection& shaderItemCollection = input.m_drawPacket->m_material->GetShaderCollection();

                if (shaderItemCollection.size() > RHI::DrawPacketBuilder::DrawItemCountMax)
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        false,
                        "Material has more than the limit of %d active shader items.",
                        RHI::DrawPacketBuilder::DrawItemCountMax);

                    // TODO: handle failure of an individual without failing the whole batch
                    return;
                }

                for (size_t shaderItemIndex = 0; shaderItemIndex < shaderItemCollection.size(); ++shaderItemIndex)
                {
                    auto& shaderItem = shaderItemCollection[shaderItemIndex];
                    if (shaderItem.IsEnabled())
                    {
                        // Skip the shader item without creating the shader instance
                        // if the mesh is not going to be rendered based on the draw tag
                        RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
                        RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

                        // Use the explicit draw list override if exists.
                        RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                        if (drawListTag.IsNull())
                        {
                            Data::Asset<RPI::ShaderAsset>& shaderAsset = shaderItem.GetShaderAssetNonConst();
                            if (!shaderAsset.IsReady())
                            {
                                // The shader asset needs to be loaded before we can check the draw tag.
                                // If it's not loaded yet, the instance database will do a blocking load
                                // when we create the instance below, so might as well load it now.
                                shaderAsset.QueueLoad();

                                if (shaderAsset.IsLoading())
                                {
                                    shaderAsset.BlockUntilLoadComplete();
                                }
                            }

                            drawListTag = drawListTagRegistry->FindTag(shaderAsset->GetDrawListName());
                        }

                        if (parentScene.HasOutputForPipelineState(drawListTag))
                        {
                            // drawListTag found in this scene, so render this item
                            activeShaderItems[inputIndex].set(shaderItemIndex);
                            activeShaderItemCount++;
                        }
                    }
                }
            }

            // Now we know the total number of drawItems we'll be creating across our entire batch of meshes
            // Capture the input and output for each one in a vector, so that we can process them as a batch.
            AZStd::vector<AppendMeshDrawPacketShaderBatchInput> appendShaderInput(activeShaderItemCount);
            AZStd::vector<AppendMeshDrawPacketShaderBatchOutput> appendShaderOutput(activeShaderItemCount);
            size_t currentAppendShaderInputIndex = 0;
            for (size_t batchInputIndex = 0; batchInputIndex < batchInput.size(); ++batchInputIndex)
            {
                auto& activeShaderItemIndices = activeShaderItems[batchInputIndex];
                ShaderCollection& shaderCollection = batchInput[batchInputIndex].m_drawPacket->m_material->GetShaderCollection();
                for (size_t bit = 0; bit < RHI::DrawPacketBuilder::DrawItemCountMax; ++bit)
                {
                    if (activeShaderItemIndices.test(bit))
                    {
                        // If the shader is active, add it to the input
                        appendShaderInput[currentAppendShaderInputIndex].shaderItem = &shaderCollection[bit];
                        appendShaderInput[currentAppendShaderInputIndex].m_modelLod = batchInput[batchInputIndex].m_drawPacket->m_modelLod;
                        appendShaderInput[currentAppendShaderInputIndex].m_prepareUpdateBatchIndex = batchInputIndex;


                        // Use the explicit draw list override if exists.
                        RHI::DrawListTag drawListTag = appendShaderInput[currentAppendShaderInputIndex].shaderItem->GetDrawListTagOverride();

                        if (drawListTag.IsNull())
                        {
                            // It's pretty annoying that we're doing this twice...but it's a way to avoid extra memory allocations
                            // The first time we did it was to count up the total number of shader items that are actually used.
                            // Now we need to track it as part of our input to the batch append shader function
                            // If this extra lookup into the drawListTagRegistry becomes a performance issue, we could alternatively
                            // explore if it's feasible to cache to drawListTag in teh shader asset itself
                           
                            Data::Asset<RPI::ShaderAsset>& shaderAsset =
                                appendShaderInput[currentAppendShaderInputIndex].shaderItem->GetShaderAssetNonConst();
                            AZ_Assert(shaderAsset.IsReady(), "The shader asset should have been loaded already");

                            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
                            RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

                            // If there is no explicit override, use the default from the shader asset
                            drawListTag = drawListTagRegistry->FindTag(shaderAsset->GetDrawListName());
                        }
                        appendShaderInput[currentAppendShaderInputIndex].m_drawListTag = drawListTag;
                        appendShaderOutput[currentAppendShaderInputIndex].m_batchInputIndex = batchInputIndex;
                        currentAppendShaderInputIndex++;
                    }
                }
                batchInput[batchInputIndex].shaderList.reserve(activeShaderItemIndices.count());
                batchInput[batchInputIndex].m_drawPacket->m_perDrawSrgs.clear();
            }


            AppendMeshDrawPacketShaderItemBatch(parentScene, batchInput, appendShaderInput, appendShaderOutput);

            for (size_t inputIndex = 0; inputIndex < batchInput.size(); ++inputIndex)
            {
                batchInput[inputIndex].m_drawPacket->m_drawPacket = batchInput[inputIndex].m_drawPacketBuilder.End();
                batchInput[inputIndex].m_drawPacket->m_materialChangeId =
                    batchInput[inputIndex].m_drawPacket->m_material->GetCurrentChangeId();
                if (batchInput[inputIndex].m_drawPacket)
                {
                    batchInput[inputIndex].m_drawPacket->m_activeShaders = batchInput[inputIndex].shaderList;
                    batchInput[inputIndex].m_drawPacket->m_materialSrg =
                        batchInput[inputIndex].m_drawPacket->m_material->GetRHIShaderResourceGroup();
                }
            }

            // TODO: handle failure of an individual without failing the whole batch
            return;
        }


        const RHI::DrawPacket* MeshDrawPacket::GetRHIDrawPacket() const
        {
            return m_drawPacket.get();
        }
    } // namespace RPI
} // namespace AZ

