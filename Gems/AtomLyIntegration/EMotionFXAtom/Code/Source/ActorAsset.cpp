/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <ActorAsset.h>
#include <AtomActorInstance.h>

#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <MCore/Source/DualQuaternion.h>

// For creating a skinned mesh from an actor
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Public/Model/Model.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/PackedVector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/Entity.h>

// Copied from ModelAssetBuilderComponent.cpp
namespace
{
    const AZ::u32 IndicesPerFace = 3;
    const AZ::RHI::Format IndicesFormat = AZ::RHI::Format::R32_UINT;

    const AZ::u32 PositionFloatsPerVert = 3;
    const AZ::u32 NormalFloatsPerVert = 3;
    const AZ::u32 UVFloatsPerVert = 2;
    const AZ::u32 ColorFloatsPerVert = 4;
    const AZ::u32 TangentFloatsPerVert = 4;
    const AZ::u32 BitangentFloatsPerVert = 3;

    const AZ::RHI::Format PositionFormat = AZ::RHI::Format::R32G32B32_FLOAT;
    const AZ::RHI::Format NormalFormat = AZ::RHI::Format::R32G32B32_FLOAT;
    const AZ::RHI::Format UVFormat = AZ::RHI::Format::R32G32_FLOAT;
    const AZ::RHI::Format ColorFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
    const AZ::RHI::Format TangentFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
    const AZ::RHI::Format BitangentFormat = AZ::RHI::Format::R32G32B32_FLOAT;
    const AZ::RHI::Format BoneIndexFormat = AZ::RHI::Format::R32G32B32A32_UINT;
    const AZ::RHI::Format BoneWeightFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;

    const size_t LinearSkinningFloatsPerBone = 12;
    const size_t DualQuaternionSkinningFloatsPerBone = 8;
    const uint32_t MaxSupportedSkinInfluences = 4;
}

namespace AZ
{
    namespace Render
    {
        static bool IsVertexCountWithinSupportedRange(size_t vertexOffset, size_t vertexCount)
        {
            return vertexOffset + vertexCount <= aznumeric_cast<size_t>(SkinnedMeshVertexStreamPropertyInterface::Get()->GetMaxSupportedVertexCount());
        }

        static void ProcessSkinInfluences(
            const EMotionFX::Mesh* mesh,
            const EMotionFX::SubMesh* subMesh,
            size_t atomVertexBufferOffset,
            AZStd::vector<uint32_t[MaxSupportedSkinInfluences / 2]>& blendIndexBufferData,
            AZStd::vector<AZStd::array<float, MaxSupportedSkinInfluences>>& blendWeightBufferData,
            bool hasClothData)
        {
            EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));
            
            // EMotionFX source gives 16 bit indices and 32 bit float weights
            // Atom consumes 32 bit uint indices and 32 bit float weights (range 0-1)
            // Up to MaxSupportedSkinInfluences influences per vertex are supported
            const uint32_t* sourceOriginalVertex = static_cast<uint32_t*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
            const uint32_t vertexCount = subMesh->GetNumVertices();
            const uint32_t vertexStart = subMesh->GetStartVertex();
            if (sourceSkinningInfo)
            {
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    const uint32_t originalVertex = sourceOriginalVertex[vertexIndex + vertexStart];
                    const uint32_t influenceCount = AZStd::GetMin<uint32_t>(MaxSupportedSkinInfluences, sourceSkinningInfo->GetNumInfluences(originalVertex));
                    uint32_t influenceIndex = 0;
                    float weightError = 1.0f;

                    AZStd::vector<uint32_t> localIndices;
                    for (; influenceIndex < influenceCount; ++influenceIndex)
                    {
                        EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                        localIndices.push_back(static_cast<uint32_t>(influence->GetNodeNr()));
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = influence->GetWeight();
                        weightError -= blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex];
                    }

                    // Zero out any unused ids/weights
                    for (; influenceIndex < MaxSupportedSkinInfluences; ++influenceIndex)
                    {
                        localIndices.push_back(0);
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = 0.0f;
                    }

                    // Now that we have the 16-bit indices, pack them into 32-bit uints
                    for (size_t i = 0; i < localIndices.size(); ++i)
                    {
                        if (i % 2 == 0)
                        {
                            // Put the first/even ids in the most significant bits
                            blendIndexBufferData[atomVertexBufferOffset + vertexIndex][i / 2] = localIndices[i] << 16;
                        }
                        else
                        {
                            // Put the next/odd ids in the least significant bits
                            blendIndexBufferData[atomVertexBufferOffset + vertexIndex][i / 2] |= localIndices[i];
                        }
                    }
                }
            }

            // [TODO ATOM-15288]
            // Temporary workaround. If there is cloth data, set all the blend weights to zero to indicate
            // the vertices will be updated by cpu. When meshes with cloth data are not dispatched for skinning
            // this can be hasClothData can be removed.

            // If there is no skinning info, default to 0 weights and display an error
            if (hasClothData || !sourceSkinningInfo)
            {
                for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    for (uint32_t influenceIndex = 0; influenceIndex < MaxSupportedSkinInfluences; ++influenceIndex)
                    {
                        blendWeightBufferData[atomVertexBufferOffset + vertexIndex][influenceIndex] = 0.0f;
                    }
                }
            }
        }
        
        static void ProcessMorphsForLod(const EMotionFX::Actor* actor, [[maybe_unused]]const Data::Asset<RPI::BufferAsset>& morphBufferAsset, uint32_t lodIndex, const AZStd::string& fullFileName, [[maybe_unused]]const SkinnedMeshInputLod& skinnedMeshLod)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodIndex);
            if (morphSetup)
            {
                AZ_Assert(actor->GetMorphTargetMetaAsset().IsReady(), "Trying to create morph targets from actor '%s', but the MorphTargetMetaAsset isn't loaded.", actor->GetName());
                const AZStd::vector<AZ::RPI::MorphTargetMetaAsset::MorphTarget>& metaDatas = actor->GetMorphTargetMetaAsset()->GetMorphTargets();

                // Loop over all the EMotionFX morph targets
                const AZ::u32 numMorphTargets = morphSetup->GetNumMorphTargets();
                for (AZ::u32 morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
                {
                    EMotionFX::MorphTargetStandard* morphTarget = static_cast<EMotionFX::MorphTargetStandard*>(morphSetup->GetMorphTarget(morphTargetIndex));
                    for (const auto& metaData : metaDatas)
                    {
                        // Loop through the metadatas to find the one that corresponds with the current morph target
                        // This ensures the order stays in sync with the order in the MorphSetup,
                        // so that the correct weights are applied to the correct morphs later
                        // Skip any that don't modify any vertices
                        if (metaData.m_morphTargetName == morphTarget->GetNameString() && metaData.m_numVertices > 0)
                        {
                            // The skinned mesh lod gets a unique morph for each meta, since each one has unique min/max delta values to use for decompression
                            AZStd::string morphString = AZStd::string::format("%s_Lod%u_Morph_%s", fullFileName.c_str(), lodIndex, metaData.m_meshNodeName.c_str());

                            [[maybe_unused]]float minWeight = morphTarget->GetRangeMin();
                            [[maybe_unused]]float maxWeight = morphTarget->GetRangeMax();

                            //skinnedMeshLod.AddMorphTarget(metaData, morphBufferAsset, morphString, minWeight, maxWeight);
                        }
                    }
                }
            }
        }

        AZStd::intrusive_ptr<SkinnedMeshInputBuffers> CreateSkinnedMeshInputFromActor(const Data::AssetId& actorAssetId, const EMotionFX::Actor* actor)
        {

            Data::Asset<RPI::ModelAsset> modelAsset = actor->GetMeshAsset();
            if (!modelAsset.IsReady())
            {
                AZ_Error("CreateSkinnedMeshInputFromActor", false, "Attempting to create skinned mesh input buffers for an actor that doesn't have a loaded model.");
                return nullptr;
            }

            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> skinnedMeshInputBuffers = aznew SkinnedMeshInputBuffers;
            skinnedMeshInputBuffers->CreateFromModelAsset(modelAsset);

            // Get the fileName, which will be used to label the buffers
            AZStd::string assetPath;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, actorAssetId);
            AZStd::string fullFileName;
            AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), fullFileName);

            // GetNumNodes returns the number of 'joints' or 'bones' in the skeleton
            const size_t numJoints = actor->GetNumNodes();
            const size_t numLODs = actor->GetNumLODLevels();

            // Create the containers to hold the data for all the combined sub-meshes
            AZStd::vector<uint32_t[MaxSupportedSkinInfluences / 2]> blendIndexBufferData;
            AZStd::vector<AZStd::array<float, MaxSupportedSkinInfluences>> blendWeightBufferData;

            //
            // Process all LODs from the EMotionFX actor data.
            //

            skinnedMeshInputBuffers->SetLodCount(numLODs);
            AZ_Assert(numLODs == modelAsset->GetLodCount(), "The lod count of the EMotionFX mesh and Atom model are out of sync for '%s'", fullFileName.c_str());
            for (size_t lodIndex = 0; lodIndex < numLODs; ++lodIndex)
            {
                // Create a single LOD
                const SkinnedMeshInputLod& skinnedMeshLod = skinnedMeshInputBuffers->GetLod(lodIndex);

                Data::Asset<RPI::ModelLodAsset> modelLodAsset = modelAsset->GetLodAssets()[lodIndex];

                // We'll be overwriting all the elements, so no need to construct them when resizing
                // TODO: operate on a per-mesh basis
                blendIndexBufferData.resize_no_construct(skinnedMeshLod.GetVertexCountForStream(SkinnedMeshOutputVertexStreams::Position));
                blendWeightBufferData.resize_no_construct(skinnedMeshLod.GetVertexCountForStream(SkinnedMeshOutputVertexStreams::Position));

                // Now iterate over the actual data and populate the data for the per-actor buffers
                size_t vertexBufferOffset = 0;
                size_t skinnedMeshSubmeshIndex = 0;
                for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
                {
                    const EMotionFX::Mesh* mesh = actor->GetMesh(lodIndex, jointIndex);
                    if (!mesh || mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    // For each sub-mesh within each mesh, we want to create a separate sub-piece.
                    const size_t numSubMeshes = mesh->GetNumSubMeshes();

                    AZ_Assert(numSubMeshes == modelLodAsset->GetMeshes().size(),
                        "Number of submeshes (%d) in EMotionFX mesh (lod %d and joint index %d) doesn't match the number of meshes (%d) in model lod asset",
                        numSubMeshes, lodIndex, jointIndex, modelLodAsset->GetMeshes().size());

                    for (size_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                    {
                        const EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                        const size_t vertexCount = subMesh->GetNumVertices();

                        // Skip empty sub-meshes and sub-meshes that would put the total vertex count beyond the supported range
                        if (vertexCount > 0 && IsVertexCountWithinSupportedRange(vertexBufferOffset, vertexCount))
                        {
                            // Check if the model mesh asset has cloth data. One ModelLodAsset::Mesh corresponds to one EMotionFX::SubMesh.
                            const bool hasClothData = modelLodAsset->GetMeshes()[subMeshIndex].GetSemanticBufferAssetView(AZ::Name("CLOTH_DATA")) != nullptr;

                            ProcessSkinInfluences(mesh, subMesh, vertexBufferOffset, blendIndexBufferData, blendWeightBufferData, hasClothData);

                            // Increment offsets so that the next sub-mesh can start at the right place
                            vertexBufferOffset += vertexCount;
                            skinnedMeshSubmeshIndex++;
                        }
                    }   // for all submeshes
                } // for all meshes

                const RPI::BufferAssetView* jointIndicesBufferView = nullptr;
                const RPI::BufferAssetView* skinWeightsBufferView = nullptr;
                const RPI::BufferAssetView* morphBufferAssetView = nullptr;
                const RPI::BufferAssetView* colorView = nullptr;

                for (const auto& modelLodMesh : modelLodAsset->GetMeshes())
                {
                    // TODO: operate on a per-mesh basis
                    
                    // If the joint id/weight buffers haven't been found on a mesh yet, keep looking
                    if (!jointIndicesBufferView)
                    {
                        jointIndicesBufferView = modelLodMesh.GetSemanticBufferAssetView(Name{ "SKIN_JOINTINDICES" });
                        if (jointIndicesBufferView)
                        {
                            skinWeightsBufferView = modelLodMesh.GetSemanticBufferAssetView(Name{ "SKIN_WEIGHTS" });
                            AZ_Error("CreateSkinnedMeshInputFromActor", skinWeightsBufferView, "Mesh '%s' on actor '%s' has joint indices but no joint weights", modelLodMesh.GetName().GetCStr(), fullFileName.c_str());
                            break;
                        }
                    }

                    // If the morph target buffer hasn't been found on a mesh yet, keep looking
                    if (!morphBufferAssetView)
                    {
                        morphBufferAssetView = modelLodMesh.GetSemanticBufferAssetView(Name{ "MORPHTARGET_VERTEXDELTAS" });
                    }

                    if (!colorView)
                    {
                        colorView = modelLodMesh.GetSemanticBufferAssetView(Name{ "COLOR" });
                    }
                }

                if (!jointIndicesBufferView || !skinWeightsBufferView)
                {
                    AZ_Error("ProcessSkinInfluences", false, "Actor '%s' lod '%zu' has no skin influences, and will be stuck in bind pose.", fullFileName.c_str(), lodIndex);
                }
                else
                {
                    Data::Asset<RPI::BufferAsset> jointIndicesBufferAsset = jointIndicesBufferView->GetBufferAsset();
                    Data::Asset<RPI::BufferAsset> skinWeightsBufferAsset = skinWeightsBufferView->GetBufferAsset();

                    // We're using the indices/weights buffers directly from the model.
                    // However, EMFX has done some re-mapping of the id's, so we need to update the GPU buffer for it to have the correct data.
                    size_t remappedJointIndexBufferSizeInBytes = blendIndexBufferData.size() * sizeof(blendIndexBufferData[0]);
                    size_t remappedSkinWeightsBufferSizeInBytes = blendWeightBufferData.size() * sizeof(blendWeightBufferData[0]);

                    AZ_Assert(jointIndicesBufferAsset->GetBufferDescriptor().m_byteCount == remappedJointIndexBufferSizeInBytes, "Joint indices data from EMotionFX is not the same size as the buffer from the model in '%s', lod '%d'", fullFileName.c_str(), lodIndex);
                    AZ_Assert(skinWeightsBufferAsset->GetBufferDescriptor().m_byteCount == remappedSkinWeightsBufferSizeInBytes, "Skin weights data from EMotionFX is not the same size as the buffer from the model in '%s', lod '%d'", fullFileName.c_str(), lodIndex);

                    if (Data::Instance<RPI::Buffer> jointIndicesBuffer = RPI::Buffer::FindOrCreate(jointIndicesBufferAsset))
                    {
                        jointIndicesBuffer->UpdateData(blendIndexBufferData.data(), remappedJointIndexBufferSizeInBytes);
                    }
                    if (Data::Instance<RPI::Buffer> skinWeightsBuffer = RPI::Buffer::FindOrCreate(skinWeightsBufferAsset))
                    {
                        skinWeightsBuffer->UpdateData(blendWeightBufferData.data(), remappedSkinWeightsBufferSizeInBytes);
                    }
                }

                if (morphBufferAssetView)
                {
                    ProcessMorphsForLod(actor, morphBufferAssetView->GetBufferAsset(), lodIndex, fullFileName, skinnedMeshLod);
                }

                // Set colors after morphs are set, so that we know whether or not they are dynamic (if they exist)
                if (colorView)
                {
                    if (skinnedMeshLod.HasDynamicColors())
                    {
                        // If colors are being morphed,
                        // add them as input to the skinning compute shader, which will apply the morph
                        //skinnedMeshLod.SetSkinningInputBufferAsset(colorView->GetBufferAsset(), SkinnedMeshInputVertexStreams::Color);
                    }
                    else
                    {
                        // If colors exist but are not modified dynamically,
                        // add them to the static streams that are shared by all instances of the same skinned mesh
                        //skinnedMeshLod.SetStaticBufferAsset(colorView->GetBufferAsset(), SkinnedMeshStaticVertexStreams::Color);
                    }
                }

            } // for all lods

            return skinnedMeshInputBuffers;
        }

        void GetBoneTransformsFromActorInstance(const EMotionFX::ActorInstance* actorInstance, AZStd::vector<float>& boneTransforms, EMotionFX::Integration::SkinningMethod skinningMethod)
        {
            const EMotionFX::TransformData* transforms = actorInstance->GetTransformData();
            const AZ::Matrix3x4* skinningMatrices = transforms->GetSkinningMatrices();

            // For linear skinning, we need a 3x4 row-major float matrix for each transform
            const size_t numBoneTransforms = transforms->GetNumTransforms();

            if (skinningMethod == EMotionFX::Integration::SkinningMethod::Linear)
            {
                boneTransforms.resize_no_construct(numBoneTransforms * LinearSkinningFloatsPerBone);
                for (size_t i = 0; i < numBoneTransforms; ++i)
                {
                    skinningMatrices[i].StoreToRowMajorFloat12(&boneTransforms[i * LinearSkinningFloatsPerBone]);
                }
            }
            else if(skinningMethod == EMotionFX::Integration::SkinningMethod::DualQuat)
            {
                boneTransforms.resize_no_construct(numBoneTransforms * DualQuaternionSkinningFloatsPerBone);
                for (size_t i = 0; i < numBoneTransforms; ++i)
                {
                    MCore::DualQuaternion dualQuat = MCore::DualQuaternion::ConvertFromTransform(AZ::Transform::CreateFromMatrix3x4(skinningMatrices[i]));
                    dualQuat.mReal.StoreToFloat4(&boneTransforms[i * DualQuaternionSkinningFloatsPerBone]);
                    dualQuat.mDual.StoreToFloat4(&boneTransforms[i * DualQuaternionSkinningFloatsPerBone + 4]);
                }
            }
        }

        Data::Instance<RPI::Buffer> CreateBoneTransformBufferFromActorInstance(const EMotionFX::ActorInstance* actorInstance, EMotionFX::Integration::SkinningMethod skinningMethod)
        {
            // Get the actual transforms
            AZStd::vector<float> boneTransforms;
            GetBoneTransformsFromActorInstance(actorInstance, boneTransforms, skinningMethod);

            size_t floatsPerBone = 0;
            if (skinningMethod == EMotionFX::Integration::SkinningMethod::Linear)
            {
                floatsPerBone = LinearSkinningFloatsPerBone;
            }
            else if (skinningMethod == EMotionFX::Integration::SkinningMethod::DualQuat)
            {
                floatsPerBone = DualQuaternionSkinningFloatsPerBone;
            }
            else
            {
                AZ_Error("ActorAsset", false, "Unsupported EMotionFX skinning method.");
            }

            // Create a buffer and populate it with the transforms
            RPI::CommonBufferDescriptor descriptor;
            descriptor.m_bufferData = boneTransforms.data();
            descriptor.m_bufferName = AZStd::string::format("BoneTransformBuffer_%s_%s", actorInstance->GetActor()->GetName(), Uuid::CreateRandom().ToString<AZStd::string>().c_str());
            descriptor.m_byteCount = boneTransforms.size() * sizeof(float);
            descriptor.m_elementSize = floatsPerBone * sizeof(float);
            descriptor.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
            return RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
        }

    } //namespace Render
} // namespace AZ
