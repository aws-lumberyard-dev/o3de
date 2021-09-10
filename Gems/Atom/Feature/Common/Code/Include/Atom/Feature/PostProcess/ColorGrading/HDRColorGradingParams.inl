/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_BOOL_PARAM(Enabled, m_enabled, false)
AZ_GFX_FLOAT_PARAM(ColorGradingExposure, m_colorGradingExposure, 0.0)
AZ_GFX_FLOAT_PARAM(ColorGradingContrast, m_colorGradingContrast, 0.0)
AZ_GFX_FLOAT_PARAM(ColorGradingHueShift, m_colorGradingHueShift, 0.0)
AZ_GFX_FLOAT_PARAM(ColorGradingPreSaturation, m_colorGradingPreSaturation, 1.0)
AZ_GFX_FLOAT_PARAM(ColorGradingFilterIntensity, m_colorGradingFilterIntensity, 1.0)
AZ_GFX_FLOAT_PARAM(ColorGradingFilterMultiply, m_colorGradingFilterMultiply, 0.0)
AZ_GFX_FLOAT_PARAM(Kelvin, m_kelvin, 15000.0)
AZ_GFX_FLOAT_PARAM(KelvinLumPreservation, m_kelvinLumPreservation, 1.0)
AZ_GFX_FLOAT_PARAM(KelvinColorMix, m_kelvinColorMix, 0.0)
AZ_GFX_FLOAT_PARAM(WhiteBalance, m_whiteBalance, 0.0)
AZ_GFX_FLOAT_PARAM(WhiteBalanceTint, m_whiteBalanceTint, 0.0)
AZ_GFX_FLOAT_PARAM(SplitToneBalance, m_splitToneBalance, 0.0)
AZ_GFX_FLOAT_PARAM(SplitToneMix, m_splitToneMix, 0.0)
AZ_GFX_FLOAT_PARAM(ColorGradingPostSaturation, m_colorGradingPostSaturation, 0.0)
AZ_GFX_FLOAT_PARAM(SmhShadowsStart, m_smhShadowsStart, 0.0)
AZ_GFX_FLOAT_PARAM(SmhShadowsEnd, m_smhShadowsEnd, 0.3)
AZ_GFX_FLOAT_PARAM(SmhHighlightsStart, m_smhHighlightsStart, 0.55)
AZ_GFX_FLOAT_PARAM(SmhHighlightsEnd, m_smhHighlightsEnd, 1.0)
AZ_GFX_FLOAT_PARAM(SmhMix, m_smhMix, 0.0)
AZ_GFX_VEC3_PARAM(ChannelMixingRed, m_channelMixingRed, AZ::Vector3(1.0f, 0.0f, 0.0f))
AZ_GFX_VEC3_PARAM(ChannelMixingGreen, m_channelMixingGreen, AZ::Vector3(0.0f, 1.0f, 0.0f))
AZ_GFX_VEC3_PARAM(ChannelMixingBlue, m_channelMixingBlue, AZ::Vector3(0.0f, 0.0f, 1.0f))
AZ_GFX_VEC4_PARAM(ColorFilterSwatch, m_colorFilterSwatch, AZ::Vector4(1.0f, 0.5f, 0.5f, 1.0f))
AZ_GFX_VEC4_PARAM(SplitToneShadowsColor, m_splitToneShadowsColor, AZ::Vector4(1.0f, 0.5f, 0.5f, 1.0f))
AZ_GFX_VEC4_PARAM(SplitToneHighlightsColor, m_splitToneHighlightsColor, AZ::Vector4(0.1f, 1.0f, 0.1f, 1.0f))
AZ_GFX_VEC4_PARAM(SmhShadowsColor, m_smhShadowsColor, AZ::Vector4(1.0f, 0.25f, 0.25f, 1.0f))
AZ_GFX_VEC4_PARAM(SmhMidtonesColor, m_smhMidtonesColor, AZ::Vector4(0.1f, 0.1f, 1.0f, 1.0f))
AZ_GFX_VEC4_PARAM(SmhHighlightsColor, m_smhHighlightsColor, AZ::Vector4(1.0f, 0.0f, 1.0f, 1.0f))
