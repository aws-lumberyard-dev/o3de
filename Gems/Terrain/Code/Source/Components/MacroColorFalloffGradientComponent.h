/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <GradientSignal/Ebuses/GradientTransformRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class MacroColorFalloffGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(MacroColorFalloffGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(MacroColorFalloffGradientConfig, "{7D940D16-44EC-455E-96A2-66E0C0ECF2F2}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_macroColorEntityId;
        AZStd::vector<AZ::Color> m_falloffColors;
        float m_falloffStartDistance = 0.05f;
        float m_falloffEndDistance = 0.10f;
    };

    inline constexpr AZ::TypeId MacroColorFalloffGradientComponentTypeId{ "{7557062C-09A8-497C-94FA-FD00406619B1}" };

    /**
    * calculates a gradient value based on distance from a shapes surface
    */      
    class MacroColorFalloffGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private Terrain::TerrainMacroMaterialNotificationBus::Handler
        , private GradientTransformNotificationBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(MacroColorFalloffGradientComponent, MacroColorFalloffGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        MacroColorFalloffGradientComponent(const MacroColorFalloffGradientConfig& configuration);
        MacroColorFalloffGradientComponent() = default;
        ~MacroColorFalloffGradientComponent() = default;

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

        //////////////////////////////////////////////////////////////////////////
        // TerrainMacroMaterialNotificationBus
        void OnTerrainMacroMaterialCreated(AZ::EntityId macroMaterialEntity, const Terrain::MacroMaterialData& macroMaterial) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId macroMaterialEntity, const Terrain::MacroMaterialData& macroMaterial) override;
        void OnTerrainMacroMaterialRegionChanged(
            AZ::EntityId macroMaterialEntity, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId macroMaterialEntity) override;

        // GradientTransformNotificationBus overrides...
        void OnGradientTransformChanged(const GradientTransform& newTransform) override;

    private:
        void ClearMacroMaterialData();
        void RefreshMacroMaterialData();

        MacroColorFalloffGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        bool m_hasValidMacroMaterial = false;
        GradientTransform m_gradientTransform;

        uint32_t m_macroColorWidth = 0;
        uint32_t m_macroColorHeight = 0;
        AZStd::vector<AZ::Color> m_macroColorPixels;
    };
}
