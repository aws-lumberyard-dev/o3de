/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/RidgedGradientComponent.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GradientSignal
{
    void RidgedGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RidgedGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Gradient", &RidgedGradientConfig::m_gradientSampler)
                ->Field("MinValue", &RidgedGradientConfig::m_minValue)
                ->Field("MirrorPoint", &RidgedGradientConfig::m_mirrorPoint)
                ->Field("MaxValue", &RidgedGradientConfig::m_maxValue)
                ->Field("InvertResults", &RidgedGradientConfig::m_invertResults)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<RidgedGradientConfig>(
                    "Ridged Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RidgedGradientConfig::m_gradientSampler, "Gradient", "Input gradient whose values will be ridged.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RidgedGradientConfig::m_minValue, "Min", "The minimum value to clamp the input to.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RidgedGradientConfig::m_mirrorPoint, "Mid", "The midpoint for flipping the input data.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RidgedGradientConfig::m_maxValue, "Max", "The maximum value to clamp the input to.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RidgedGradientConfig::m_invertResults, "Invert Results", "Whether or not to invert the final result.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RidgedGradientConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("gradientSampler", BehaviorValueProperty(&RidgedGradientConfig::m_gradientSampler))
                ;
        }
    }

    void RidgedGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void RidgedGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void RidgedGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void RidgedGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        RidgedGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RidgedGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &RidgedGradientComponent::m_configuration)
                ;
        }
    }

    RidgedGradientComponent::RidgedGradientComponent(const RidgedGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void RidgedGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_gradientSampler.m_gradientId);

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RidgedGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
    }

    bool RidgedGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RidgedGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool RidgedGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<RidgedGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float RidgedGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float value = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&sampleParams.m_position, 1), AZStd::span<float>(&value, 1));
        return value;
    }

    void RidgedGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        m_configuration.m_gradientSampler.GetValues(positions, outValues);

        // The following constants reduce the number of calculations per-value, but also make the math a bit harder to read.
        // This is a generalization of the formula "1 - abs(2 * value - 1)", which scales our input gradient from [0, 1] 
        // to [-1, 1], mirrors the bottom half of the range via the abs value, and then inverts the results. This formula creates
        // sharp "ridges" by forcing peaks at 1.
        // The generalized version gives us:
        // - customized mirror cutoff point so that we can have asymmetric scaling and peaking
        // - customized min/max range values so that the scaling on each side of the mirror point can be controlled even further
        // - optional inversion so that it can produce valleys at 0 instead of peaks at 1

        // The constants combine the initial scaling equations and the optional inversion together.
        // The basic formulas are:
        //      (mirror - value) / (mirror - min)           value < mirror, no inversion
        //      (value - mirror) / (max - mirror)           value >= mirror, no inversion
        //      1 - (mirror - value) / (mirror - min)       value < mirror, inversion
        //      1 - (value - mirror) / (max - mirror)       value >= mirror, inversion
        // To minimize the per-value computation, we rearrange the formulas into this:
        //      0 + (1 / (mirror - min)) * (mirror - value)     value < mirror, no inversion
        //      0 + (1 / (max - mirror)) * (value - mirror)     value >= mirror, no inversion
        //      1 + (-1 / (mirror - min)) * (mirror - value)    value < mirror, inversion
        //      1 + (-1 / (max - mirror)) * (value - mirror)    value >= mirror, inversion

        const float invertAdder = (m_configuration.m_invertResults) ? 1.0f : 0.0f;
        const float invertMultiplier = (m_configuration.m_invertResults) ? -1.0f : 1.0f;

        const float belowMirrorPointReciprocal = (m_configuration.m_mirrorPoint == m_configuration.m_minValue)
            ? 0.0f
            : invertMultiplier / (m_configuration.m_mirrorPoint - m_configuration.m_minValue);

        const float aboveMirrorPointReciprocal = (m_configuration.m_mirrorPoint == m_configuration.m_maxValue)
            ? 0.0f
            : invertMultiplier / (m_configuration.m_maxValue - m_configuration.m_mirrorPoint);


        for (auto& outValue : outValues)
        {
            if (outValue < m_configuration.m_mirrorPoint)
            {
                outValue = invertAdder + ((m_configuration.m_mirrorPoint - outValue) * belowMirrorPointReciprocal);
            }
            else
            {
                outValue = invertAdder + ((outValue - m_configuration.m_mirrorPoint) * aboveMirrorPointReciprocal);
            }

            outValue = AZStd::clamp(outValue, 0.0f, 1.0f);
        }
    }

    bool RidgedGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        return m_configuration.m_gradientSampler.IsEntityInHierarchy(entityId);
    }

    GradientSampler& RidgedGradientComponent::GetGradientSampler()
    {
        return m_configuration.m_gradientSampler;
    }
}
