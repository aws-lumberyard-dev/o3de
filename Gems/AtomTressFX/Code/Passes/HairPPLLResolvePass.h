/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <TressFX/TressFXConstantBuffers.h>
#include <Rendering/HairCommon.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            static const char* const HairPPLLResolvePassTemplateName = "HairPPLLResolvePassTemplate";
            class HairFeatureProcessor;

            //! The hair PPLL resolve pass is a full screen pass that runs over the hair fragments list
            //!  that were computed in the raster fill pass and resolves their depth order, transparency
            //! and lighting values to be output to display.
            //! Each pixel on the screen will processed only once and will iterate through the fragments
            //!  list associated with the pixel's location.
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

                HairFeatureProcessor* m_featureProcessor = nullptr;
                AZ::RPI::ShaderVariantKey m_shaderOptions;
            };

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
