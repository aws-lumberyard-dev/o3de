/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace Ocean
{
    class OceanFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
    {
    public:

        AZ_RTTI(OceanFeatureProcessor, "{B9B80747-8969-4D85-A1DF-0E01E9CAE9C4}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(OceanFeatureProcessor);
        AZ_FEATURE_PROCESSOR(OceanFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);


        struct OceanSettings
        {
            uint32_t m_textureSize = 256;
            float m_gravity = 9.81f;
            float m_depth = 500.0f;
        };


        OceanFeatureProcessor() = default;
        ~OceanFeatureProcessor() = default;

        // AZ::RPI::FeatureProcessor overrides...
        void Activate() override;
        void Deactivate() override;
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket & packet) override;

        OceanSettings GetOceanSettings();

    private:

        // AZ::RPI::FeatureProcessor overrides...
        void ApplyRenderPipelineChange(AZ::RPI::RenderPipeline* renderPipeline) override;

        OceanSettings m_oceanSettings;

    };
} // namespace Ocean
