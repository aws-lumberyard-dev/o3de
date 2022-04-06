/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OceanFeatureProcessor.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace Ocean
{

    void OceanFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OceanFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void OceanFeatureProcessor::Activate()
    {
        AZ_Printf("OceanFeatureProcessor", "OCEAN ACTIVATE!");
    }

    void OceanFeatureProcessor::Deactivate()
    {

    }

    void OceanFeatureProcessor::ApplyRenderPipelineChange(AZ::RPI::RenderPipeline* renderPipeline)
    {
        // Get the pass request to create ocean parent pass from the asset
        const char* passRequestAssetFilePath = "Passes/OceanParentPassRequest.azasset";
        auto passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        const AZ::RPI::PassRequest* passRequest = nullptr;
        if (passRequestAsset->IsReady())
        {
            passRequest = passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
        }
        if (!passRequest)
        {
            AZ_Error("Ocean", false, "Failed to add ocean parent pass. Can't load PassRequest from %s", passRequestAssetFilePath);
            return;
        }

        // Return if the pass to be created already exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        AZ::RPI::Pass* pass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (pass)
        {
            return;
        }

        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> oceanParentPass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!oceanParentPass)
        {
            AZ_Error("Ocean", false, "Create ocean parent pass from pass request failed");
            return;
        }

        // Add the pass to render pipeline
        bool success = renderPipeline->AddPassBefore(oceanParentPass, AZ::Name("DepthPrePass"));
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error("Ocean", false, "Add the ocean parent pass to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());
        }
    }

    void OceanFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet [[maybe_unused]] )
    {

    }

    auto OceanFeatureProcessor::GetOceanSettings() -> OceanSettings
    {
        return m_oceanSettings;
    }
}
