/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/MacroColorFalloffGradientComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Asset/AssetManager.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

#pragma optimize("", off)

namespace GradientSignal
{
    void MacroColorFalloffGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MacroColorFalloffGradientConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("MacroColorEntityId", &MacroColorFalloffGradientConfig::m_macroColorEntityId)
                ->Field("FalloffColors", &MacroColorFalloffGradientConfig::m_falloffColors)
                ->Field("FalloffStartDistance", &MacroColorFalloffGradientConfig::m_falloffStartDistance)
                ->Field("FalloffEndDistance", &MacroColorFalloffGradientConfig::m_falloffEndDistance)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<MacroColorFalloffGradientConfig>(
                    "Macro Color Falloff Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        0, &MacroColorFalloffGradientConfig::m_macroColorEntityId, "Macro Color Entity Id",
                        "Entity with macro color component to get source color from.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("TerrainMacroMaterialProviderService"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &MacroColorFalloffGradientConfig::m_falloffColors, "Falloff Colors",
                        "Colors to compute falloff from.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &MacroColorFalloffGradientConfig::m_falloffStartDistance, "Falloff Start Distance",
                        "Falloff start. Everything closer to the color than this will go to 1 in the gradient.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &MacroColorFalloffGradientConfig::m_falloffEndDistance, "Falloff End Distance",
                        "Falloff end. Everything further from the color than this will go to 0 in the gradient.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            }
        }
    }

    void MacroColorFalloffGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void MacroColorFalloffGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void MacroColorFalloffGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void MacroColorFalloffGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        MacroColorFalloffGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MacroColorFalloffGradientComponent, AZ::Component>()
                ->Version(0)->Field(
                "Configuration", &MacroColorFalloffGradientComponent::m_configuration)
                ;
        }
    }

    MacroColorFalloffGradientComponent::MacroColorFalloffGradientComponent(const MacroColorFalloffGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void MacroColorFalloffGradientComponent::Activate()
    {
        // This will immediately call OnGradientTransformChanged and initialize m_gradientTransform.
        GradientTransformNotificationBus::Handler::BusConnect(GetEntityId());

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_macroColorEntityId);
        Terrain::TerrainMacroMaterialNotificationBus::Handler::BusConnect();

        RefreshMacroMaterialData();

        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MacroColorFalloffGradientComponent::Deactivate()
    {
        GradientRequestBus::Handler::BusDisconnect();

        ClearMacroMaterialData();

        GradientTransformNotificationBus::Handler::BusDisconnect();
        m_dependencyMonitor.Reset();
        Terrain::TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
    }

    bool MacroColorFalloffGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const MacroColorFalloffGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool MacroColorFalloffGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<MacroColorFalloffGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void MacroColorFalloffGradientComponent::OnGradientTransformChanged(const GradientTransform& newTransform)
    {
        m_gradientTransform = newTransform;
    }

    float MacroColorFalloffGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float value = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&(sampleParams.m_position), 1), AZStd::span<float>(&value, 1));
        return value;
    }

    void MacroColorFalloffGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        for (auto& outValue : outValues)
        {
            outValue = 0.0f;
        }

        if (m_macroColorPixels.empty())
        {
            return;
        }

        auto width = m_macroColorWidth;
        auto height = m_macroColorHeight;

        if (width > 0 && height > 0)
        {
            const AZ::Vector3 tiledDimensions(aznumeric_cast<float>(width), aznumeric_cast<float>(height), 0.0f);

            constexpr bool useLinearColors = false;
            AZStd::vector<AZ::Color> falloffColors;
            falloffColors.reserve(m_configuration.m_falloffColors.size());
            for (auto& falloffColor : m_configuration.m_falloffColors)
            {
                if (useLinearColors)
                {
                    falloffColors.push_back(falloffColor.GammaToLinear());
                }
                else
                {
                    falloffColors.push_back(falloffColor);
                }
            }

            for (size_t index = 0; index < positions.size(); index++)
            {
                AZ::Vector3 uvw = positions[index];
                bool wasPointRejected = false;
                m_gradientTransform.TransformPositionToUVWNormalized(positions[index], uvw, wasPointRejected);

                if (!wasPointRejected)
                {
                    // Convert from uv space back to pixel space
                    AZ::Vector3 pixelLookup = (uvw * tiledDimensions);

                    auto x = aznumeric_cast<AZ::u32>(pixelLookup.GetX()) % width;
                    auto y = aznumeric_cast<AZ::u32>(pixelLookup.GetY()) % height;

                    // Flip the y because images are stored in reverse of our world axes
                    y = (height - 1) - y;

                    float minDistanceSquared = 1.0f;
                    for (auto& falloffColor : falloffColors)
                    {
                        float distanceSquared = 0.0f;

                        // Compute RGB distance
                        const uint8_t numComponents = 3;
                        for (uint8_t channel = 0; channel < numComponents; channel++)
                        {
                            float pixel = m_macroColorPixels[y * width + x].GetElement(channel);
                            float channelDist = (falloffColor.GetElement(channel) - pixel);
                            distanceSquared += channelDist * channelDist;
                        }
                        minDistanceSquared = AZStd::min(minDistanceSquared, distanceSquared);
                    }

                    float distance = sqrtf(minDistanceSquared);
                    outValues[index] = (distance - m_configuration.m_falloffStartDistance) /
                        (m_configuration.m_falloffEndDistance - m_configuration.m_falloffStartDistance);
                    outValues[index] = 1.0f - AZ::GetClamp(outValues[index], 0.0f, 1.0f);
                }
            }
        }
    }

    void MacroColorFalloffGradientComponent::ClearMacroMaterialData()
    {
        m_macroColorHeight = 0;
        m_macroColorWidth = 0;
        m_macroColorPixels.clear();
    }

    void MacroColorFalloffGradientComponent::RefreshMacroMaterialData()
    {
        ClearMacroMaterialData();

        Terrain::TerrainMacroMaterialRequestBus::Event(
            m_configuration.m_macroColorEntityId, &Terrain::TerrainMacroMaterialRequestBus::Events::GetTerrainMacroMaterialColorData,
            m_macroColorWidth, m_macroColorHeight, m_macroColorPixels);
    }

    void MacroColorFalloffGradientComponent::OnTerrainMacroMaterialCreated(
        [[maybe_unused]] AZ::EntityId macroMaterialEntity, [[maybe_unused]] const Terrain::MacroMaterialData& macroMaterial)
    {
        if (macroMaterialEntity == m_configuration.m_macroColorEntityId)
        {
            RefreshMacroMaterialData();
        }
    }

    void MacroColorFalloffGradientComponent::OnTerrainMacroMaterialChanged(
        [[maybe_unused]] AZ::EntityId macroMaterialEntity, [[maybe_unused]] const Terrain::MacroMaterialData& macroMaterial)
    {
        if (macroMaterialEntity == m_configuration.m_macroColorEntityId)
        {
            RefreshMacroMaterialData();
        }
    }

    void MacroColorFalloffGradientComponent::OnTerrainMacroMaterialRegionChanged(
        [[maybe_unused]] AZ::EntityId macroMaterialEntity,
        [[maybe_unused]] const AZ::Aabb& oldRegion,
        [[maybe_unused]] const AZ::Aabb& newRegion)
    {
        if (macroMaterialEntity == m_configuration.m_macroColorEntityId)
        {
            RefreshMacroMaterialData();
        }
    }

    void MacroColorFalloffGradientComponent::OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId macroMaterialEntity)
    {
        if (macroMaterialEntity == m_configuration.m_macroColorEntityId)
        {
            ClearMacroMaterialData();
        }
    }

}

#pragma optimize("", on)
