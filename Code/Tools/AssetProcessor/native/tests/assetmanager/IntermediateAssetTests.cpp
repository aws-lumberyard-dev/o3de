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
    void IntermediateAssetTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        using namespace AssetBuilderSDK;

        CreateJobFunction createJobsStage1 = [](const CreateJobsRequest&, CreateJobsResponse& response)
        {
            response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", "stage 1", CommonPlatformName });
            response.m_result = CreateJobsResultCode::Success;
        };

        ProcessJobFunction processJobStage1 = [](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension("stage1");

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName("stage1"), 1 };

            product.m_outputFlags = ProductOutputFlags::IntermediateAsset;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            response.m_resultCode = ProcessJobResult_Success;
        };

        m_builderInfoHandler.CreateBuilderDesc(
            "stage 1", AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.input", AssetBuilderPattern::Wildcard } }, createJobsStage1, processJobStage1, "fingerprint");

        CreateJobFunction createJobsStage2 = [](const CreateJobsRequest& request, CreateJobsResponse& response)
        {
            for (const auto& platform : request.m_enabledPlatforms)
            {
                response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", "stage 1", platform.m_identifier.c_str() });
            }
            response.m_result = CreateJobsResultCode::Success;
        };

        ProcessJobFunction processJobStage2 = [](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension("stage2");

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName("stage2"), 2 };

            product.m_outputFlags = ProductOutputFlags::ProductAsset;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            response.m_resultCode = ProcessJobResult_Success;
        };

        m_builderInfoHandler.CreateBuilderDesc(
            "stage 2", AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.stage1", AssetBuilderPattern::Wildcard } }, createJobsStage2, processJobStage2, "fingerprint");
    }

    void IntermediateAssetTests::TearDown()
    {
        AssetManagerTestingBase::TearDown();
    }

    TEST_F(IntermediateAssetTests, FileProcessedAsIntermediateIntoProduct)
    {
        using namespace AssetBuilderSDK;

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.input";
        AZStd::string testFilePath = (scanFolderDir / testFilename).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(testFilePath.c_str(), "unit test file");

        AssetProcessor::RCController rc(1, 1);
        rc.SetDispatchPaused(false);

        AZStd::atomic_bool fileCompiled = false;
        AssetProcessor::JobEntry jobEntry;
        ProcessJobResponse response;
        QObject::connect(
            &rc, &AssetProcessor::RCController::FileCompiled,
            [&fileCompiled, &jobEntry, &response](auto entryIn, auto responseIn)
            {
                fileCompiled = true;
                jobEntry = entryIn;
                response = responseIn;
            });

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, testFilePath.c_str()));
        QCoreApplication::processEvents();

        AZStd::vector<AssetProcessor::JobDetails> jobDetailsList;
        RunFile(jobDetailsList);
        ProcessJob(rc, jobDetailsList[0]);

        ASSERT_TRUE(fileCompiled);

        m_assetProcessorManager->AssetProcessed(jobEntry, response);

        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";
        auto intermediatesDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);
        auto expectedIntermediatePath = intermediatesDir / "test.stage1";
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str())) << expectedIntermediatePath.c_str();

        // stage1 file should be processed now, and it should have queued up stage2
        jobDetailsList.clear();
        RunFile(jobDetailsList);

        fileCompiled = false;
        ProcessJob(rc, jobDetailsList[0]);
        ASSERT_TRUE(fileCompiled);

        m_assetProcessorManager->AssetProcessed(jobEntry, response);

        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckJobEntries(0);

        auto expectedProductPath = cacheDir / "pc" / "test.stage2";
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedProductPath.c_str())) << expectedProductPath.c_str();
    }
} // namespace UnitTests
