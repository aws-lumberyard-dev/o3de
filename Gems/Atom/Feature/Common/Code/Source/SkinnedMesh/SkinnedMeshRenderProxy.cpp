/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <MorphTargets/MorphTargetComputePass.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/Utils/Utils.h>
namespace AZ
{
    namespace Render
    {
        SkinnedMeshRenderProxy::SkinnedMeshRenderProxy(const SkinnedMeshFeatureProcessorInterface::SkinnedMeshRenderProxyDesc& desc)
            : m_inputBuffers(desc.m_inputBuffers)
            , m_instance(desc.m_instance)
            , m_meshHandle(desc.m_meshHandle)
            , m_boneTransforms(desc.m_boneTransforms)
            , m_shaderOptions(desc.m_shaderOptions)
        {
        }

        bool SkinnedMeshRenderProxy::Init(const RPI::Scene& scene, SkinnedMeshFeatureProcessor* featureProcessor)
        {
            AZ_PROFILE_FUNCTION(AzRender);
            if(!m_instance->m_model)
            {
                return false;
            }
            const size_t modelLodCount = m_instance->m_model->GetLodCount();
            m_featureProcessor = featureProcessor;

            m_dispatchItemsByLod.reserve(modelLodCount);
            for (uint32_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                if (!BuildDispatchItem(scene, modelLodIndex, m_shaderOptions))
                {
                    return false;
                }
            }
            
            return true;
        }

        static AZStd::vector<float> CalculateMorphTargetIntegerEncodingsForLod(uint32_t modelLodIndex, Data::Instance<SkinnedMeshInputBuffers> skinnedMeshInputBuffers)
        {
            uint32_t meshCount = skinnedMeshInputBuffers->GetMeshCount(modelLodIndex);
            AZStd::vector<float> morphDeltaIntegerEncodings;
            morphDeltaIntegerEncodings.reserve(meshCount);

            for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                // Get the value needed for encoding/decoding floats as integers when passing them
                // from the morph target pass to the skinning pass.
                // Integer encoding is used so that AZSL's InterlockedAdd can be used, which only supports int/uint
                const AZStd::vector<MorphTargetComputeMetaData>& morphTargetComputeMetaDatas =
                    skinnedMeshInputBuffers->GetMorphTargetComputeMetaDatas(modelLodIndex, meshIndex);

                // Verify the morph targets were created correctly for this mesh
                // With an equal number of metadatas and input buffers
                const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& morphTargetInputBuffersVector =
                    skinnedMeshInputBuffers->GetMorphTargetInputBuffers(modelLodIndex, meshIndex);
                AZ_Assert(
                    morphTargetComputeMetaDatas.size() == morphTargetInputBuffersVector.size(),
                    "Skinned Mesh Feature Processor - Mismatch in morph target metadata count and morph target input buffer count");

                float morphDeltaIntegerEncoding = 0.0f;
                if (morphTargetComputeMetaDatas.size() > 0)
                {
                    morphDeltaIntegerEncoding = ComputeMorphTargetIntegerEncoding(morphTargetComputeMetaDatas);
                }

                // Keep track of the integer encoding for this mesh, so it can be used when adding morph targets later
                morphDeltaIntegerEncodings.push_back(morphDeltaIntegerEncoding);
            }

            return morphDeltaIntegerEncodings;
        }

        bool SkinnedMeshRenderProxy::BuildDispatchItem([[maybe_unused]] const RPI::Scene& scene, uint32_t modelLodIndex, [[maybe_unused]] const SkinnedMeshShaderOptions& shaderOptions)
        {
            Data::Instance<RPI::Shader> skinningShader = m_featureProcessor->GetSkinningShader();
            if (!skinningShader)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get skinning shader from skinning pass");
                return false;
            }

            // Get the data needed to create a morph target dispatch item
            Data::Instance<RPI::Shader> morphTargetShader = m_featureProcessor->GetMorphTargetShader();
            if (!morphTargetShader)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get morph target shader from morph target pass");
                return false;
            }

            // Create a vector of dispatch items for each lod
            m_dispatchItemsByLod.emplace_back(AZStd::vector<AZStd::unique_ptr<SkinnedMeshDispatchItem>>());
            m_morphTargetDispatchItemsByLod.emplace_back(AZStd::vector<AZStd::unique_ptr<MorphTargetDispatchItem>>());

            AZStd::vector<float> morphDeltaIntegerEncodings = CalculateMorphTargetIntegerEncodingsForLod(modelLodIndex, m_inputBuffers);

            // Populate the vector with a dispatch item for each mesh
            for (uint32_t meshIndex = 0; meshIndex < m_inputBuffers->GetMeshCount(modelLodIndex); ++meshIndex)
            {
                // Create the skinning dispatch Item
                m_dispatchItemsByLod[modelLodIndex].emplace_back(
                    aznew SkinnedMeshDispatchItem{
                        m_inputBuffers,
                        m_instance->m_outputStreamOffsetsInBytes[modelLodIndex][meshIndex],
                        m_instance->m_positionHistoryBufferOffsetsInBytes[modelLodIndex][meshIndex],
                        modelLodIndex, meshIndex, m_boneTransforms,
                        m_shaderOptions,
                        m_featureProcessor,
                        m_instance->m_morphTargetInstanceMetaData[modelLodIndex][meshIndex],
                        morphDeltaIntegerEncodings[meshIndex] });
            }

            AZ_Assert(m_dispatchItemsByLod.size() == modelLodIndex + 1, "Skinned Mesh Feature Processor - Mismatch in size between the fixed vector of dispatch items and the lod being initialized");
            for (uint32_t meshIndex = 0; meshIndex < m_inputBuffers->GetMeshCount(modelLodIndex); ++meshIndex)
            {
                if (!m_dispatchItemsByLod[modelLodIndex][meshIndex]->Init())
                {
                    return false;
                }
            }
            
            // Now loop over the morph targets create the morph target dispatch items
            const AZStd::vector<SkinnedMeshInputLod::MorphIndex>& morphTargetDispatchItemOrder =
                m_inputBuffers->GetMorphTargetDispatchOrder(modelLodIndex);

            // Create one dispatch item per morph target, in the order that they were originally added
            // to the skinned mesh to stay in sync with the animation system
            for (const SkinnedMeshInputLod::MorphIndex& morphTargetIndex : morphTargetDispatchItemOrder)
            {
                m_morphTargetDispatchItemsByLod[modelLodIndex].emplace_back(aznew MorphTargetDispatchItem{
                    m_inputBuffers->GetMorphTargetInputBuffers(modelLodIndex, morphTargetIndex.m_meshIndex)[morphTargetIndex.m_morphIndex],
                    m_inputBuffers->GetMorphTargetComputeMetaDatas(modelLodIndex, morphTargetIndex.m_meshIndex)[morphTargetIndex.m_morphIndex],
                    m_featureProcessor,
                    m_instance->m_morphTargetInstanceMetaData[modelLodIndex][morphTargetIndex.m_meshIndex],
                    morphDeltaIntegerEncodings[morphTargetIndex.m_meshIndex] });

                // Initialize the MorphTargetDispatchItem we just created
                if (!m_morphTargetDispatchItemsByLod[modelLodIndex].back()->Init())
                {
                    return false;
                }
            }
            
            return true;
        }

        void SkinnedMeshRenderProxy::SetTransform(const AZ::Transform& transform)
        {
            // Set the position to be used for determining lod
            m_position = transform.GetTranslation();
        }

        void SkinnedMeshRenderProxy::SetSkinningMatrices(const AZStd::vector<float>& data)
        {
            if (m_boneTransforms)
            {
                WriteToBuffer(m_boneTransforms->GetRHIBuffer(), data);
            }
        }

        void SkinnedMeshRenderProxy::SetMorphTargetWeights(uint32_t lodIndex, const AZStd::vector<float>& weights)
        {
            auto& morphTargetDispatchItems = m_morphTargetDispatchItemsByLod[lodIndex];

            AZ_Assert(morphTargetDispatchItems.size() == weights.size(), "Skinned Mesh Feature Processor - Morph target weights passed into SetMorphTargetWeight don't align with morph target dispatch items.");
            for (size_t morphIndex = 0; morphIndex < weights.size(); ++morphIndex)
            {
                morphTargetDispatchItems[morphIndex]->SetWeight(weights[morphIndex]);
            }
        }

        uint32_t SkinnedMeshRenderProxy::GetLodCount() const
        {
            return aznumeric_caster(m_dispatchItemsByLod.size());
        }

        AZStd::span<const AZStd::unique_ptr<SkinnedMeshDispatchItem>> SkinnedMeshRenderProxy::GetDispatchItems(uint32_t lodIndex) const
        {
            return m_dispatchItemsByLod[lodIndex];
        }

    } // namespace Render
} // namespace AZ
