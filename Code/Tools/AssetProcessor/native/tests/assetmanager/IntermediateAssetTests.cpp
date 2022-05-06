/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/IntermediateAssetTests.h>

namespace UnitTests
{
    AssetBuilderSDK::CreateJobFunction CreateJobStage(const AZStd::string& name, bool commonPlatform)
    {
        using namespace AssetBuilderSDK;

        // Note: capture by copy because we need these to stay around for a long time
        return [name, commonPlatform]([[maybe_unused]] const CreateJobsRequest& request, CreateJobsResponse& response)
        {
            if (commonPlatform)
            {
                response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, CommonPlatformName });
            }
            else
            {
                for (const auto& platform : request.m_enabledPlatforms)
                {
                    response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, platform.m_identifier.c_str() });
                }
            }
            response.m_result = CreateJobsResultCode::Success;
        };
    }

    AssetBuilderSDK::ProcessJobFunction ProcessJobStage(const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags)
    {
        using namespace AssetBuilderSDK;

        // Capture by copy because we need these to stay around a long time
        return [outputExtension, flags](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension(outputExtension.c_str());

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName(outputExtension.c_str()), 1 };

            product.m_outputFlags = flags;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            response.m_resultCode = ProcessJobResult_Success;
        };
    }

    void IntermediateAssetTests::CreateBuilder(const char* name, const char* inputFilter, bool createJobCommonPlatform, AssetBuilderSDK::ProductOutputFlags outputFlags)
    {
        using namespace AssetBuilderSDK;

        m_builderInfoHandler.CreateBuilderDesc(
            name, AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ inputFilter, AssetBuilderPattern::Wildcard } }, CreateJobStage(name, createJobCommonPlatform),
            ProcessJobStage(name, outputFlags), "fingerprint");
    }

    void IntermediateAssetTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.input";
        m_testFilePath = (scanFolderDir / testFilename).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(m_testFilePath.c_str(), "unit test file");

        m_rc = AZStd::make_unique<AssetProcessor::RCController>(1, 1);

        m_rc->SetDispatchPaused(false);

        QObject::connect(
            m_rc.get(), &AssetProcessor::RCController::FileFailed,
            [this](auto entryIn)
            {
                m_fileFailed = true;
            });

        QObject::connect(
            m_rc.get(), &AssetProcessor::RCController::FileCompiled,
            [this](auto jobEntry, auto response)
            {
                m_fileCompiled = true;
                m_processedJobEntry = jobEntry;
                m_processJobResponse = response;
            });

        m_localFileIo->SetAlias("@log@", (AZ::IO::Path(m_tempDir.GetDirectory()) / "logs").c_str());
    }

    void IntermediateAssetTests::IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.input", commonPlatform, flags);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        AZStd::vector<AssetProcessor::JobDetails> jobDetailsList;
        RunFile(jobDetailsList);
        ProcessJob(*m_rc, jobDetailsList[0]);

        ASSERT_TRUE(m_fileFailed);
    }

    void IntermediateAssetTests::TearDown()
    {
        AssetManagerTestingBase::TearDown();
    }

    TEST_F(IntermediateAssetTests, FileProcessedAsIntermediateIntoProduct)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.input", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage1", false, ProductOutputFlags::ProductAsset);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        AZStd::vector<AssetProcessor::JobDetails> jobDetailsList;
        RunFile(jobDetailsList);
        ProcessJob(*m_rc, jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";
        auto intermediatesDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);
        auto expectedIntermediatePath = intermediatesDir / "test.stage1";
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str())) << expectedIntermediatePath.c_str();

        // stage1 file should be processed now, and it should have queued up stage2
        jobDetailsList.clear();
        RunFile(jobDetailsList);

        m_fileCompiled = false;
        ProcessJob(*m_rc, jobDetailsList[0]);
        ASSERT_TRUE(m_fileCompiled);

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckJobEntries(0);

        auto expectedProductPath = cacheDir / "pc" / "test.stage2";
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedProductPath.c_str())) << expectedProductPath.c_str();
    }

    TEST_F(IntermediateAssetTests, IntermediateOutputWithWrongPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset);
    }

    TEST_F(IntermediateAssetTests, ProductOutputWithWrongPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(true, AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, IntermediateAndProductOutputFlags_NormalPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset | AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, IntermediateAndProductOutputFlags_CommonPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(true, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset | AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, NoFlags_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, (AssetBuilderSDK::ProductOutputFlags)(0));
    }
} // namespace UnitTests
