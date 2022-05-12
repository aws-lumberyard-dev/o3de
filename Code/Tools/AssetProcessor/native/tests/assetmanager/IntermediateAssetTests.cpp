/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/IntermediateAssetTests.h>
#include <QCoreApplication>
#include <native/unittests/UnitTestRunner.h>

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

    AssetBuilderSDK::ProcessJobFunction ProcessJobStage(const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags, bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        // Capture by copy because we need these to stay around a long time
        return [outputExtension, flags, outputExtraFile](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension(outputExtension.c_str());

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName(outputExtension.c_str()), 1 };

            product.m_outputFlags = flags;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            if (outputExtraFile)
            {
                auto extraFilePath = AZ::IO::Path(request.m_tempDirPath) / "z_extra.txt"; // Z prefix to place at end of list when sorting for processing

                UnitTestUtils::CreateDummyFile(extraFilePath.c_str(), "unit test file");

                auto extraProduct = JobProduct{ extraFilePath.c_str(), AZ::Data::AssetType::CreateName("extra"), 2 };

                extraProduct.m_outputFlags = flags;
                extraProduct.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(extraProduct);
            }

            response.m_resultCode = ProcessJobResult_Success;
        };
    }

    void IntermediateAssetTests::CreateBuilder(const char* name, const char* inputFilter, const char* outputExtension, bool createJobCommonPlatform, AssetBuilderSDK::ProductOutputFlags outputFlags, bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        m_builderInfoHandler.CreateBuilderDesc(
            name, AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ inputFilter, AssetBuilderPattern::Wildcard } }, CreateJobStage(name, createJobCommonPlatform),
            ProcessJobStage(outputExtension, outputFlags, outputExtraFile), "fingerprint");
    }

    void IntermediateAssetTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage1";
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

        CreateBuilder("stage1", "*.stage1", "stage2", commonPlatform, flags);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        RunFile(1);
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileFailed);
    }

    void IntermediateAssetTests::TearDown()
    {
        AssetManagerTestingBase::TearDown();
    }

    TEST_F(IntermediateAssetTests, FileProcessedAsIntermediateIntoProduct)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2","stage3", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(2, true);
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

    AZStd::string IntermediateAssetTests::MakePath(const char* filename, bool intermediate)
    {
        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";

        if (intermediate)
        {
            cacheDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

            return (cacheDir / filename).StringAsPosix();
        }

        return (cacheDir / "pc" / filename).StringAsPosix();
    }

    void IntermediateAssetTests::CheckProduct(const char* relativePath, bool exists)
    {
        auto expectedProductPath = MakePath(relativePath, false);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedProductPath.c_str()), exists) << expectedProductPath.c_str();
    }

    void IntermediateAssetTests::CheckIntermediate(const char* relativePath, bool exists)
    {
        auto expectedIntermediatePath = MakePath(relativePath, true);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str()), exists) << expectedIntermediatePath.c_str();
    }

    void IntermediateAssetTests::ProcessFileMultiStage(int endStage, bool doProductOutputCheck, const char* file, int startStage, bool expectAutofail, bool hasExtraFile)
    {
        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";
        auto intermediatesDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

        if (file == nullptr)
        {
            file = m_testFilePath.c_str();
        }

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, file));
        QCoreApplication::processEvents();

        for (int i = startStage; i <= endStage; ++i)
        {
            // Reset state
            m_jobDetailsList.clear();
            m_fileCompiled = false;
            m_fileFailed = false;

            // If there's an extra file output, it'll only show up after the 1st iteration
            if (i > startStage && hasExtraFile)
            {
                RunFile(2);
            }
            else
            {
                RunFile(expectAutofail ? 2 : 1);
            }

            int jobToRun = expectAutofail ? 1 : 0;

            std::stable_sort(
                m_jobDetailsList.begin(), m_jobDetailsList.end(),
                [](const AssetProcessor::JobDetails& a, const AssetProcessor::JobDetails& b) -> bool
                {
                    return a.m_jobEntry.m_databaseSourceName.compare(b.m_jobEntry.m_databaseSourceName) < 0;
                });

            ProcessJob(*m_rc, m_jobDetailsList[jobToRun]);

            ASSERT_TRUE(m_fileCompiled);

            m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

            if (i < endStage)
            {
                auto expectedIntermediatePath = intermediatesDir / AZStd::string::format("test.stage%d", i + 1);
                EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str())) << expectedIntermediatePath.c_str();
            }

            // Only first job should have an autofail due to a conflict
            expectAutofail = false;
        }

        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckJobEntries(0);

        if (doProductOutputCheck)
        {
            CheckProduct(AZStd::string::format("test.stage%d", endStage + 1).c_str());
        }
    }

    TEST_F(IntermediateAssetTests, ABALoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage2",  true, ProductOutputFlags::IntermediateAsset); // Loop back to an intermediate

        ProcessFileMultiStage(3, false);

        EXPECT_EQ(m_jobDetailsList.size(), 3);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);
        EXPECT_TRUE(m_jobDetailsList[2].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage3");
        EXPECT_EQ(m_jobDetailsList[2].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, AALoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage1", true, ProductOutputFlags::IntermediateAsset); // Loop back to the source

        ProcessFileMultiStage(2, false);

        EXPECT_EQ(m_jobDetailsList.size(), 3);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);
        EXPECT_TRUE(m_jobDetailsList[2].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage2");
        EXPECT_EQ(m_jobDetailsList[2].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, SelfLoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage1", true, ProductOutputFlags::IntermediateAsset); // Loop back to the source with a single job

        ProcessFileMultiStage(1, false);

        EXPECT_EQ(m_jobDetailsList.size(), 2);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, CopyJob_Works)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage1", false, ProductOutputFlags::ProductAsset); // Copy jobs are ok

        ProcessFileMultiStage(1, false);

        auto expectedProduct = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache" / "pc" / "test.stage1";

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedProduct.c_str())) << expectedProduct.c_str();
    }

    TEST_F(IntermediateAssetTests, DeleteSourceIntermediate_DeletesAllProducts)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        AZ::IO::SystemFile::Delete(m_testFilePath.c_str());
        m_assetProcessorManager->AssessDeletedFile(m_testFilePath.c_str());
        RunFile(0);

        CheckIntermediate("test.stage2", false);
        CheckIntermediate("test.stage3", false);
        CheckProduct("test.stage4", false);
    }

    TEST_F(IntermediateAssetTests, DeleteIntermediateProduct_Reprocesses)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        auto stage2Path = MakePath("test.stage2", true);

        AZ::IO::SystemFile::Delete(stage2Path.c_str());
        m_assetProcessorManager->AssessDeletedFile(stage2Path.c_str());
        RunFile(0); // Process the delete

        RunFile(1); // Reprocess the file
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        CheckIntermediate("test.stage3");
    }

    TEST_F(IntermediateAssetTests, DeleteFinalProduct_Reprocesses)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        auto stage4Path = MakePath("test.stage4", false);

        AZ::IO::SystemFile::Delete(stage4Path.c_str());
        m_assetProcessorManager->AssessDeletedFile(stage4Path.c_str());
        RunFile(0); // Process the delete

        RunFile(1); // Reprocess the file
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        CheckProduct("test.stage4");
    }

    TEST_F(IntermediateAssetTests, Override_NormalFileProcessedFirst_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        // Make and process a source file which matches an intermediate output name we will create later
        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage2";
        AZStd::string testFilePath = (scanFolderDir / testFilename).AsPosix();

        UnitTestUtils::CreateDummyFile(testFilePath.c_str(), "unit test file");

        ProcessFileMultiStage(3, true, testFilePath.c_str(), 2);

        // Now process another file which produces intermediates that conflict with the existing source file above
        // Only go to stage 1 since we're expecting a failure at that point
        ProcessFileMultiStage(1, false);

        EXPECT_EQ(m_jobDetailsList.size(), 2);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, Override_IntermediateFileProcessedFirst_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        // Process a file from stage1 -> stage4, this will create several intermediates
        ProcessFileMultiStage(3, true);

        // Now make a source file which is the same name as an existing intermediate and process it
        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage2";
        AZStd::string testFilePath = (scanFolderDir / testFilename).AsPosix();

        UnitTestUtils::CreateDummyFile(testFilePath.c_str(), "unit test file");

        ProcessFileMultiStage(3, true, testFilePath.c_str(), 2, true);

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_FALSE(m_jobDetailsList[0].m_autoFail);
    }

    TEST_F(IntermediateAssetTests, DuplicateOutputs_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset, true);
        CreateBuilder("stage2", "*.stage2", "stage3", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(2, true, nullptr, 1, false, true);

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test2.stage1";

        UnitTestUtils::CreateDummyFile((scanFolderDir / testFilename).c_str(), "unit test file");

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, (scanFolderDir / testFilename).c_str()));
        QCoreApplication::processEvents();

        RunFile(1);
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_jobDetailsList.clear();

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_TRUE(m_jobDetailsList[0].m_autoFail);
    }
} // namespace UnitTests
