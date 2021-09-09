/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/HDRColorGradingPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<HDRColorGradingPass> HDRColorGradingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<HDRColorGradingPass> pass = aznew HDRColorGradingPass(descriptor);
            return AZStd::move(pass);
        }
        
        HDRColorGradingPass::HDRColorGradingPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void HDRColorGradingPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_colorGradingExposureIndex.Reset();
            m_colorGradingPostSaturationIndex.Reset();
            //m_colorGradingContrastIndex.Reset();
            //m_colorGradingHueShiftIndex.Reset();
            //m_colorGradingPreSaturationIndex.Reset();
            // TODO: Add other indices
        }
        
        void HDRColorGradingPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void HDRColorGradingPass::SetSrgConstants()
        {
            const HDRColorGradingSettings* settings = GetHDRColorGradingSettings();
            if (settings)
            {
                m_shaderResourceGroup->SetConstant(m_colorGradingExposureIndex, 0.0f);
                m_shaderResourceGroup->SetConstant(m_colorGradingPostSaturationIndex, settings->GetColorGradingPostSaturation());
            }
        }

        const AZ::Render::HDRColorGradingSettings* HDRColorGradingPass::GetHDRColorGradingSettings() const
        {
            RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        return postProcessSettings->GetHDRColorGradingSettings();
                    }
                }
            }
            return nullptr;
        }

        void HDRColorGradingPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            FullscreenTrianglePass::CompileResources(context);
        }
    }
}
