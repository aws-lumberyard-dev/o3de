/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <utilities/AssetUtilEBusHelper.h>
#include <utilities/assetUtils.h>

namespace UnitTests
{
    struct MockMultiBuilderInfoHandler : public AssetProcessor::AssetBuilderInfoBus::Handler
    {
        ~MockMultiBuilderInfoHandler() override;

        struct AssetBuilderExtraInfo
        {
            QString m_jobFingerprint;
            QString m_dependencyFilePath;
            QString m_jobDependencyFilePath;
            QString m_analysisFingerprint;
            AZStd::vector<AZ::u32> m_subIdDependencies;
        };

        //! AssetProcessor::AssetBuilderInfoBus Interface
        void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
        void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

        void CreateJobs(
            AssetBuilderExtraInfo extraInfo,
            const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(
            AssetBuilderExtraInfo extraInfo,
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response);

        void CreateBuilderDesc(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderExtraInfo& extraInfo);

        // Use this version if you intend to update the extraInfo struct dynamically (be sure extraInfo does not go out of scope)
        void CreateBuilderDescInfoRef(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderExtraInfo& extraInfo);

        void CreateBuilderDesc(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderSDK::CreateJobFunction& createJobsFunction,
            const AssetBuilderSDK::ProcessJobFunction& processJobFunction,
            AZStd::optional<QString> analysisFingerprint = AZStd::nullopt);

        AZStd::vector<AssetUtilities::BuilderFilePatternMatcher> m_matcherBuilderPatterns;
        AZStd::unordered_map<AZ::Uuid, AssetBuilderSDK::AssetBuilderDesc> m_builderDescMap;

        int m_createJobsCount = 0;
    };
}
