/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

//#include <Atom/RHI/CommandList.h>
//#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <TressFX/TressFXConstantBuffers.h>
#include <Rendering/HairCommon.h>
#include <Rendering/HairGlobalSettings.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            static const char* const HairPPLLResolvePassTemplateName = "HairPPLLResolvePassTemplate";
            class HairFeatureProcessor;

            // This is the hair PPLL resolve pass that is a full screen pass that runs over all
            // saved pixel samples rendered by the hair and resolve their depth order, transparency
            // and lighting values and output to display.
            //! The full screen resolve pass is using the following Srgs:
            //!  - PerPassSrg: hair vertex data data, PPLL buffers and material array
            //!     shared by all passes.
            class HairPPLLResolvePass final
                : public RPI::FullscreenTrianglePass
            {
                AZ_RPI_PASS(HairPPLLResolvePass);

            public:
                AZ_RTTI(HairPPLLResolvePass, "{240940C1-4A47-480D-8B16-176FF3359B01}", RPI::FullscreenTrianglePass);
                AZ_CLASS_ALLOCATOR(HairPPLLResolvePass, SystemAllocator, 0);
                ~HairPPLLResolvePass() = default;

                static RPI::Ptr<HairPPLLResolvePass> Create(const RPI::PassDescriptor& descriptor);

                bool AcquireFeatureProcessor();

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }

                //! Override pass behavior methods
                void InitializeInternal() override;
                void BuildInternal() override;
                void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            private:
                HairPPLLResolvePass(const RPI::PassDescriptor& descriptor);

            private:
                void UpdateGlobalShaderOptions();

                HairGlobalSettings m_hairGlobalSettings;
                HairFeatureProcessor* m_featureProcessor = nullptr;
                AZ::RPI::ShaderVariantKey m_shaderOptions;
            };


        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
