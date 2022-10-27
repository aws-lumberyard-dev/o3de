/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <GradientSignal/SmoothStep.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class GradientDerivativeConfig
        : public AZ::ComponentConfig
    {
    public:
        enum class RampType : AZ::u8
        {
            LINEAR_RAMP_DOWN,
            LINEAR_RAMP_UP,
            SMOOTH_STEP
        };

        AZ_CLASS_ALLOCATOR(GradientDerivativeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientDerivativeConfig, "{7428A1A8-BB90-491B-9B1F-FE72EF9517CE}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        float m_slopeMin = 0.0f;
        float m_slopeMax = 20.0f;
        RampType m_rampType = RampType::LINEAR_RAMP_DOWN;
        SmoothStep m_smoothStep;
        GradientSampler m_gradientSampler;

        float m_slopeQuerySpacing = 0.5f;
        float m_gradientToWorldScale = 512.0f;

        bool IsSmoothStepReadOnly() const
        {
            return (m_rampType != RampType::SMOOTH_STEP);
        }
        AZ::Crc32 GetSmoothStepParameterVisibility() const
        {
            return (m_rampType == RampType::SMOOTH_STEP) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }
    };

    inline constexpr AZ::TypeId GradientDerivativeComponentTypeId{ "{A162C8DC-B124-495C-9992-573E5F08D835}" };

    /**
    * calculates a gradient value by calculating the difference between adjacent gradient values.
    */      
    class GradientDerivativeComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(GradientDerivativeComponent, GradientDerivativeComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        GradientDerivativeComponent(const GradientDerivativeConfig& configuration);
        GradientDerivativeComponent() = default;
        ~GradientDerivativeComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

    protected:
        GradientSampler& GetGradientSampler();

    private:

        GradientDerivativeConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
