/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        HDRColorGradingSettings::HDRColorGradingSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void HDRColorGradingSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void HDRColorGradingSettings::ApplySettingsTo(HDRColorGradingSettings* target, [[maybe_unused]] float alpha) const
        {
            AZ_Assert(target != nullptr, "HDRColorGradingSettings::ApplySettingsTo called with nullptr as argument.");

            target->m_colorGradingPostSaturation = AZ::Lerp(target->m_colorGradingPostSaturation, m_colorGradingPostSaturation, alpha);
        }

        void HDRColorGradingSettings::Simulate([[maybe_unused]] float deltaTime)
        {
        }
    } // namespace Render
} // namespace AZ


