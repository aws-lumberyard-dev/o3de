/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

#include <AzCore/Component/TickBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainGradientColorMapping final
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainGradientColorMapping, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainGradientColorMapping, "{C9655FA9-B451-4845-8613-36555612A092}");
        static void Reflect(AZ::ReflectContext* context);

        TerrainGradientColorMapping() = default;
        TerrainGradientColorMapping(const AZ::EntityId& maskEntityId, const AZ::Color& color1, const AZ::Color& color2)
            : m_maskEntityId(maskEntityId)
            , m_color1(color1)
            , m_color2(color2)
        {
        }

        AZ::EntityId m_maskEntityId;
        AZ::Color m_color1;
        AZ::Color m_color2;
    };


    class TerrainGradientMacroMaterialConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainGradientMacroMaterialConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainGradientMacroMaterialConfig, "{E4FCF25B-04F6-4533-A79A-381D18362D72}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector2 m_imageResolution;
        AZ::EntityId m_variationEntityId;
        AZ::Color m_baseColor1;
        AZ::Color m_baseColor2;
        AZStd::vector<TerrainGradientColorMapping> m_gradientColorMappings;
    };

    class TerrainGradientMacroMaterialComponent
        : public AZ::Component
        , public TerrainMacroMaterialRequestBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainGradientMacroMaterialComponent, "{70FB6C12-91E0-4697-8BF9-4168FED95E06}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainGradientMacroMaterialComponent(const TerrainGradientMacroMaterialConfig& configuration);
        TerrainGradientMacroMaterialComponent() = default;
        ~TerrainGradientMacroMaterialComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        MacroMaterialData GetTerrainMacroMaterialData() override;
        void GetTerrainMacroMaterialColorData(uint32_t& width, uint32_t& height, AZStd::vector<AZ::Color>& pixels) override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        void QueueRefresh();

        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        void HandleMaterialStateChange();

        void GenerateMacroMaterial();

        TerrainGradientMacroMaterialConfig m_configuration;
        AZ::Aabb m_cachedShapeBounds;
        bool m_macroMaterialActive{ false };
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_generatedImage;

        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        AZStd::vector<uint32_t> m_cachedPixels;
        int m_cachedPixelsHeight = 0;
        int m_cachedPixelsWidth = 0;
    };
}
