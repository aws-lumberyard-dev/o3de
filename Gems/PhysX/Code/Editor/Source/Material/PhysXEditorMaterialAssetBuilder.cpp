/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request,
        [[]] AssetBuilderSDK::ProcessJobResponse& response) const
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        /*
        AZStd::string physxMaterialFilename = request.m_sourceFile;
        AZStd::string physicsMaterialFilename = request.m_sourceFile;
        AzFramework::StringFunc::Path::ReplaceExtension(physicsMaterialFilename, "physicsmaterial");

        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_productFileName = physicsMaterialFilename;
        jobProduct.m_productAssetType = azrtti_typeid<Physics::MaterialAsset>();
        jobProduct.m_productSubID = 0;
        response.m_outputProducts.push_back(AZStd::move(jobProduct));

        AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset;

        if (!SerializeOutMaterialAsset(physicsMaterialAsset, request, response))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        */
    }

    /*bool EditorMaterialAssetBuilder::SerializeOutMaterialAsset(
        AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset,
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZStd::string physxMaterialFilename = request.m_sourceFile;
        AZStd::string physicsMaterialFilename = request.m_sourceFile;
        AzFramework::StringFunc::Path::ReplaceExtension(physicsMaterialFilename, "physicsmaterial");

        AZStd::string shaderAssetFileName = AZStd::string::format("%s.%s", shaderAsset->GetName().GetCStr(), RPI::ShaderAsset::Extension);
        AZStd::string shaderAssetOutputPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.data(), shaderAssetFileName.data(), shaderAssetOutputPath, true);

        if (!Utils::SaveObjectToFile(shaderAssetOutputPath, DataStream::ST_BINARY, shaderAsset.Get()))
        {
            AZ_Error(ShaderAssetBuilderName, false, "Failed to output Shader Descriptor");
            return false;
        }

        AssetBuilderSDK::JobProduct materialJobProduct;
        if (!AssetBuilderSDK::OutputObject(physicsMaterialAsset.Get(), shaderAssetOutputPath, azrtti_typeid<RPI::ShaderAsset>(),
            aznumeric_cast<uint32_t>(RPI::ShaderAssetSubId::ShaderAsset), materialJobProduct))
        {
            AZ_Error(ShaderAssetBuilderName, false, "Failed to output product dependencies.");
            return false;
        }
        response.m_outputProducts.push_back(AZStd::move(shaderJobProduct));

        return true;
    }*/
} // PhysX
