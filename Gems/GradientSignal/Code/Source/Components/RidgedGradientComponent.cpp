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
        for (auto& outValue : outValues)
        {
            if (outValue < m_configuration.m_mirrorPoint)
            {
                outValue = (m_configuration.m_mirrorPoint - outValue) / (m_configuration.m_mirrorPoint - m_configuration.m_minValue);
            }
            else
            {
                outValue = (outValue - m_configuration.m_mirrorPoint) / (m_configuration.m_maxValue - m_configuration.m_mirrorPoint);
            }

            outValue = AZStd::clamp(outValue, 0.0f, 1.0f);

            if (m_configuration.m_invertResults)
            {
                outValue = 1.0f - outValue;
            }
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
