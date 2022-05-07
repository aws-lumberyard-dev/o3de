/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/tests/assetmanager/AssetManagerTestingBase.h>

namespace UnitTests
{
    struct IntermediateAssetTests : AssetManagerTestingBase
    {
        void SetUp() override;
        void TearDown() override;

        void IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags);

        void CreateBuilder(
            const char* name,
            const char* inputFilter,
            const char* outputExtension,
            bool createJobCommonPlatform,
            AssetBuilderSDK::ProductOutputFlags outputFlags);

        void ProcessFileMultiStage(int stageCount, bool doProductOutputCheck);

        AZStd::unique_ptr<AssetProcessor::RCController> m_rc;
        bool m_fileCompiled = false;
        bool m_fileFailed = false;
        AssetProcessor::JobEntry m_processedJobEntry;
        AssetBuilderSDK::ProcessJobResponse m_processJobResponse;
        AZStd::string m_testFilePath;
    };
} // namespace UnitTests
