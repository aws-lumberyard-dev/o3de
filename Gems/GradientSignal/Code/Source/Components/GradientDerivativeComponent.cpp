/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/GradientDerivativeComponent.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GradientSignal
{
    void GradientDerivativeConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientDerivativeConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Gradient", &GradientDerivativeConfig::m_gradientSampler)
                ->Field("SlopeQuerySpacing", &GradientDerivativeConfig::m_slopeQuerySpacing)
                ->Field("GradientToWorldScale", &GradientDerivativeConfig::m_gradientToWorldScale)
                ->Field("SlopeMin", &GradientDerivativeConfig::m_slopeMin)
                ->Field("SlopeMax", &GradientDerivativeConfig::m_slopeMax)
                ->Field("RampType", &GradientDerivativeConfig::m_rampType)
                ->Field("SmoothStep", &GradientDerivativeConfig::m_smoothStep);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientDerivativeConfig>(
                    "Gradient Derivative", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientDerivativeConfig::m_gradientSampler,
                        "Gradient", "Input gradient whose values will be transformed.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientDerivativeConfig::m_slopeQuerySpacing,
                        "Query Spacing", "Distance in meters for querying points to calculate the derivative.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientDerivativeConfig::m_gradientToWorldScale,
                        "World Scale", "Convert the gradient values to world space values.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientDerivativeConfig::m_slopeMin,
                        "Slope Min", "Minimum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientDerivativeConfig::m_slopeMax,
                        "Slope Max", "Maximum surface slope angle in degrees.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 90.0f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GradientDerivativeConfig::m_rampType,
                        "Ramp Type", "Type of ramp to apply to the slope.")
                    ->EnumAttribute(GradientDerivativeConfig::RampType::LINEAR_RAMP_DOWN, "Linear Ramp Down")
                    ->EnumAttribute(GradientDerivativeConfig::RampType::LINEAR_RAMP_UP, "Linear Ramp Up")
                    ->EnumAttribute(GradientDerivativeConfig::RampType::SMOOTH_STEP, "Smooth Step")
                    // Note: ReadOnly doesn't currently propagate to children, so instead we hide/show smooth step parameters when
                    // we change the ramp type.  If ReadOnly is ever changed to propagate downwards, we should change the next line
                    // to PropertyRefreshLevels::AttributesAndLevels and change the Visibility line below on m_smoothStep
                    // to AZ::Edit::PropertyVisibility::Show.
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &GradientDerivativeConfig::m_smoothStep,
                        "Smooth Step Settings",
                        "Parameters for controlling the smooth-step curve.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GradientDerivativeConfig::GetSmoothStepParameterVisibility)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &GradientDerivativeConfig::IsSmoothStepReadOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false);
            }
        }
    }

    void GradientDerivativeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void GradientDerivativeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void GradientDerivativeComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void GradientDerivativeComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientDerivativeConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientDerivativeComponent, AZ::Component>()
                ->Version(0)->Field(
                "Configuration", &GradientDerivativeComponent::m_configuration)
                ;
        }
    }

    GradientDerivativeComponent::GradientDerivativeComponent(const GradientDerivativeConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void GradientDerivativeComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void GradientDerivativeComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
    }

    bool GradientDerivativeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const GradientDerivativeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool GradientDerivativeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<GradientDerivativeConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float GradientDerivativeComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float value = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&sampleParams.m_position, 1), AZStd::span<float>(&value, 1));
        return value;
    }

    void GradientDerivativeComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        const float angleMin = AZ::DegToRad(AZ::GetClamp(m_configuration.m_slopeMin, 0.0f, 90.0f));
        const float angleMax = AZ::DegToRad(AZ::GetClamp(m_configuration.m_slopeMax, 0.0f, 90.0f));

        // To calculate derivatives, we need to query gradient positions around the requested value position.
        // This controls how far apart we'll space those queries.
        const float exactRange = m_configuration.m_slopeQuerySpacing;

        // The number of points that we're querying for each normal.
        const size_t queryCount = 4;

        // The full set of positions to query to be able to calculate all the normals.
        AZStd::vector<AZ::Vector3> queryPositions;
        queryPositions.reserve(positions.size() * queryCount);

        for (const auto& position : positions)
        {
            // For each input position, query the four outer points of the + sign.
            queryPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY() - exactRange, 0.0f)); // down
            queryPositions.emplace_back(AZ::Vector3(position.GetX() - exactRange, position.GetY(), 0.0f)); // left
            queryPositions.emplace_back(AZ::Vector3(position.GetX() + exactRange, position.GetY(), 0.0f)); // right
            queryPositions.emplace_back(AZ::Vector3(position.GetX(), position.GetY() + exactRange, 0.0f)); // up
        }

        // These constants are the relative index for each of the four positions that we pushed for each input above.
        constexpr size_t down = 0;
        constexpr size_t left = 1;
        constexpr size_t right = 2;
        constexpr size_t up = 3;

        AZStd::vector<float> gradientValues(queryPositions.size());

        const float doubledRange = exactRange * 2.0f;
        const float doubledRangeWithGradientScale = doubledRange * m_configuration.m_gradientToWorldScale;

        // Fill in the outValues with all of the generated inupt gradient values.
        m_configuration.m_gradientSampler.GetValues(queryPositions, gradientValues);

        for (size_t inPosIndex = 0, queryPositionIndex = 0; inPosIndex < positions.size(); inPosIndex++, queryPositionIndex += queryCount)
        {
            // We have 4 vertices that make a + sign, cross the two lines to get the normal at the center.
            // (right - left) x (up - down)
            // Since the 4 vertices are equally spaced, calculating the normal reduces down to the following.
            AZ::Vector3 normal =
                AZ::Vector3(
                    (gradientValues[queryPositionIndex + left] - gradientValues[queryPositionIndex + right]) * doubledRangeWithGradientScale,
                    (gradientValues[queryPositionIndex + down] - gradientValues[queryPositionIndex + up]) * doubledRangeWithGradientScale,
                    doubledRange * doubledRange
                    ).GetNormalized();

            const float slope = normal.GetZ();
            // Convert slope back to an angle so that we can lerp in "angular space", not "slope value space".
            // (We want our 0-1 range to be linear across the range of angles)
            const float slopeAngle = acosf(slope);

            switch (m_configuration.m_rampType)
            {
            case GradientDerivativeConfig::RampType::SMOOTH_STEP:
                outValues[inPosIndex] = m_configuration.m_smoothStep.GetSmoothedValue(GetRatio(angleMin, angleMax, slopeAngle));
                break;
            case GradientDerivativeConfig::RampType::LINEAR_RAMP_UP:
                // For ramp up, linearly interpolate from min to max.
                outValues[inPosIndex] = AZ::GetClamp((slopeAngle - angleMin) / (angleMax - angleMin), 0.0f, 1.0f);
                break;
            case GradientDerivativeConfig::RampType::LINEAR_RAMP_DOWN:
            default:
                // For ramp down, linearly interpolate from max to min.
                outValues[inPosIndex] = AZ::GetClamp((slopeAngle - angleMax) / (angleMin - angleMax), 0.0f, 1.0f);
                break;
            }
        }
    }

    bool GradientDerivativeComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    GradientSampler& GradientDerivativeComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
