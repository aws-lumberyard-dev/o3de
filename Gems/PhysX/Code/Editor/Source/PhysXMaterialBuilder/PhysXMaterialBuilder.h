/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <PhysXMaterial/MaterialAsset/PhysXMaterialAsset.h>
#include <AzCore/JSON/document.h>

namespace AZ
{
    namespace PhysX
    {
        class MaterialBuilder final
            : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
        {
        public:
            AZ_TYPE_INFO(AZ::PhysX::MaterialBuilder, "{D63C4E94-F9B8-4F42-98F3-FBC2740649A9}");

            static const char* JobKey;

            MaterialBuilder() = default;
            ~MaterialBuilder();

            // Asset Builder Callback Functions
            void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
            void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

            // AssetBuilderSDK::AssetBuilderCommandBus interface
            void ShutDown() override;

            /// Register to builder and listen to builder command
            void RegisterBuilder();

        private:
            
            AZ::Data::Asset<MaterialAsset> CreateMaterialAsset(AZStd::string_view materialSourceFilePath, const rapidjson::Value& json) const;
            bool ReportMaterialAssetWarningsAsErrors() const;
            
            bool m_isShuttingDown = false;
        };

    } // namespace RPI
} // namespace AZ
