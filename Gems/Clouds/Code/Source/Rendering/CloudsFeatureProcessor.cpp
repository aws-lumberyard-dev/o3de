/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Rendering/CloudsFeatureProcessor.h>

#include <Passes/CloudsFullScreenPass.h>

namespace Clouds
{
    CloudsFeatureProcessor::CloudsFeatureProcessor()
    {
    }

    CloudsFeatureProcessor::~CloudsFeatureProcessor()
    {
    }

    void CloudsFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->Class<CloudsFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(1);
        }
    }

    void CloudsFeatureProcessor::Activate()
    {
        EnableSceneNotification();
    }

    void CloudsFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
    }

    void AddPassRequestToRenderPipeline(
        AZ::RPI::RenderPipeline* renderPipeline,
        const char* passRequestAssetFilePath,
        const char* referencePass, bool beforeReferencePass)
    {
        auto passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
            passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
        const AZ::RPI::PassRequest* passRequest = nullptr;
        if (passRequestAsset->IsReady())
        {
            passRequest = passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
        }
        if (!passRequest)
        {
            AZ_Error("Clouds", false, "Can't load PassRequest from %s", passRequestAssetFilePath);
            return;
        }

        // Return if the pass to be created already exists
        AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
        AZ::RPI::Pass* existingPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
        if (existingPass)
        {
            return;
        }

        // Create the pass
        AZ::RPI::Ptr<AZ::RPI::Pass> newPass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
        if (!newPass)
        {
            AZ_Error("Clouds", false, "Failed to create the pass from pass request [%s].", passRequest->m_passName.GetCStr());
            return;
        }

        // Add the pass to render pipeline
        bool success;
        if (beforeReferencePass)
        {
            success = renderPipeline->AddPassBefore(newPass, AZ::Name(referencePass));
        }
        else
        {
            success = renderPipeline->AddPassAfter(newPass, AZ::Name(referencePass));
        }
        // only create pass resources if it was success
        if (!success)
        {
            AZ_Error(
                "Clouds", false, "Failed to add pass [%s] to render pipeline [%s].", newPass->GetName().GetCStr(),
                renderPipeline->GetId().GetCStr());
        }
    }

    void CloudsFeatureProcessor::ApplyRenderPipelineChange(AZ::RPI::RenderPipeline* renderPipeline)
    {
        AddPassRequestToRenderPipeline(renderPipeline, "Passes/CloudsFullScreenPassRequest.azasset", "SkyBoxPass", /*before*/ true);
    }

    void CloudsFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
    {
        AZ_PROFILE_FUNCTION(AzRender);
        AZ_UNUSED(packet);
    }

    void CloudsFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_FUNCTION(AzRender);
    }

    void CloudsFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] AZ::RPI::RenderPipelinePtr renderPipeline)
    {
    }

    void CloudsFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
    }

    void CloudsFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
    }

} // namespace Clouds

#pragma optimize("", on)

