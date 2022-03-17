/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/TestIntermediateAssetBuilderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace TestAssetBuilder
{
    void TestIntermediateAssetBuilderComponent::Init()
    {
    }

    void TestIntermediateAssetBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Test Intermediate Asset Builder";
        builderDescriptor.m_version = 1;
        builderDescriptor.m_patterns.emplace_back("*.intersource", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDescriptor.m_busId = azrtti_typeid<TestIntermediateAssetBuilderComponent>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&TestIntermediateAssetBuilderComponent::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&TestIntermediateAssetBuilderComponent::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void TestIntermediateAssetBuilderComponent::Deactivate()
    {
        BusDisconnect();
    }

    void TestIntermediateAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestIntermediateAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void TestIntermediateAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TestIntermediateAssetBuilderPluginService"));
    }

    void TestIntermediateAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TestIntermediateAssetBuilderPluginService"));
    }

    void TestIntermediateAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestIntermediateAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TestIntermediateAssetBuilderComponent::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        auto commonPlatform = AZStd::find_if(request.m_enabledPlatforms.begin(), request.m_enabledPlatforms.end(), [](auto input) -> bool
        {
            return input.m_identifier == AssetBuilderSDK::CommonPlatformName;
        });

        if(commonPlatform != request.m_enabledPlatforms.end())
        {
            AZ_Error("TestIntermediateAssetBuilder", false, "Common platform was found in the list of enabled platforms."
                "  This is not expected as it will cause all builders to output files for the common platform.");
            return;
        }

        AssetBuilderSDK::JobDescriptor desc{"", "Test Intermediate", AssetBuilderSDK::CommonPlatformName};
        response.m_createJobOutputs.push_back(desc);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void TestIntermediateAssetBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        AZ::IO::FileIOBase* fileIO = AZ::IO::LocalFileIO::GetInstance();
        AZStd::string outputData;

        AZ::IO::HandleType sourcefileHandle;
        if (!fileIO->Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead, sourcefileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file ( %s ).", request.m_fullPath.c_str());
            return;
        }

        AZ::u64 sourceSizeBytes = 0;
        if (fileIO->Size(sourcefileHandle, sourceSizeBytes))
        {
            outputData.resize(sourceSizeBytes);
            if (!fileIO->Read(sourcefileHandle, outputData.data(), sourceSizeBytes))
            {
                AZ_Error("AssetBuilder", false, " Unable to read file ( %s ).", request.m_fullPath.c_str());
                fileIO->Close(sourcefileHandle);
                return;
            }
        }

        fileIO->Close(sourcefileHandle);

        AZStd::string fileName = request.m_sourceFile.c_str();
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);
        AZ::Data::AssetType outputAssetType = AZ::Data::AssetType::CreateNull();

        AZ::u32 outputSubId = 0;

        AzFramework::StringFunc::Path::ReplaceExtension(fileName, "source");

        // write the file to the cache at (temppath)/filenameOnly
        AZStd::string destPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(fileName.c_str(), fileNameOnly); // this removes the path from fileName.
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), destPath, true);

        // Check if we are cancelled or shutting down before doing intensive processing on this source file
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::IO::HandleType assetfileHandle;

        if (!fileIO->Open(destPath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, assetfileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file for writing ( %s ).", destPath.c_str());
            return;
        }

        if (!fileIO->Write(assetfileHandle, outputData.data(), outputData.size()))
        {
            AZ_Error("AssetBuilder", false, " Unable to write to file data ( %s ).", destPath.c_str());
            fileIO->Close(assetfileHandle);
            return;
        }
        fileIO->Close(assetfileHandle);

        AssetBuilderSDK::JobProduct jobProduct(fileNameOnly, outputAssetType, outputSubId);
        jobProduct.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies

        // once you've filled up the details of the product in jobProduct, add it to the result list:
        response.m_outputProducts.push_back(jobProduct);

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

    }

    void TestIntermediateAssetBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

} // namespace TestAssetBuilder

