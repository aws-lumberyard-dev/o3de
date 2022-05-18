/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <AssetBuilderSDK/SerializationDependencies.h>

#include <Editor/Source/Material/PhysXEditorMaterialAssetBuilder.h>

namespace PhysX
{
    void EditorMaterialAssetBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request,
        AssetBuilderSDK::CreateJobsResponse& response) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "PhysX Material Asset";
            jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void EditorMaterialAssetBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset = CreatePhysicsMaterialAsset(request);
        if (!physicsMaterialAsset.IsReady())
        {
            AZ_Error("EditorMaterialAssetBuilder", false, "Failed to create physics material assset.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        if (!SerializeOutPhysicsMaterialAsset(physicsMaterialAsset, request, response))
        {
            AZ_Error("EditorMaterialAssetBuilder", false, "Failed to serialize out physics material asset.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    AZ::Data::Asset<Physics::MaterialAsset> EditorMaterialAssetBuilder::CreatePhysicsMaterialAsset(
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request) const
    {
        // TODO: Create map from PhysX Material Asset
        const AZStd::unordered_map<AZStd::string, float> defaultMaterialProperties =
        {
            {"DynamicFriction", 0.5f},
            {"StaticFriction", 0.5f},
            {"Restitution", 0.5f},
            {"Density", 1.0f}
        };

        AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset(AZ::Uuid::CreateRandom(), aznew Physics::MaterialAsset, AZ::Data::AssetLoadBehavior::PreLoad);
        physicsMaterialAsset->SetData(defaultMaterialProperties);
        return physicsMaterialAsset;
    }

    bool EditorMaterialAssetBuilder::SerializeOutPhysicsMaterialAsset(
        AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset,
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AZStd::string physicsMaterialFilename = request.m_sourceFile;
        AzFramework::StringFunc::Path::ReplaceExtension(physicsMaterialFilename, Physics::MaterialAsset::FileExtension);

        AZStd::string physicsMaterialAssetOutputPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), physicsMaterialFilename.c_str(), physicsMaterialAssetOutputPath, true);

        if (!AZ::Utils::SaveObjectToFile(physicsMaterialAssetOutputPath, AZ::DataStream::ST_JSON, physicsMaterialAsset.Get()))
        {
            AZ_Error("EditorMaterialAssetBuilder", false, "Failed to save physics material asset to file: %s", physicsMaterialAssetOutputPath.c_str());
            return false;
        }

        AssetBuilderSDK::JobProduct physicsMaterialJobProduct;
        if (!AssetBuilderSDK::OutputObject(
            physicsMaterialAsset.Get(),
            physicsMaterialAssetOutputPath,
            azrtti_typeid<Physics::MaterialAsset>(),
            Physics::MaterialAsset::AssetSubId,
            physicsMaterialJobProduct))
        {
            AZ_Error("EditorMaterialAssetBuilder", false, "Failed to output product dependencies.");
            return false;
        }

        response.m_outputProducts.push_back(AZStd::move(physicsMaterialJobProduct));
        return true;
    }
} // PhysX
