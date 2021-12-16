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
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <Terrain/Ebuses/CoordinateMapperRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class CoordinateMapperConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(CoordinateMapperConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(CoordinateMapperConfig, "{C682CA04-A67F-4966-B6E8-097E7EC2CB7B}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector2 m_originLatLong{ 0.0f, 0.0f };

        // NOTE:  The lowest and highest points on Earth are -10911 m at the Mariana Trench, and 8848 m on top of Mt Everest.
        AZ::Vector2 m_minMaxWorldHeights{ 0.0f, 1024.0f };
        float m_scale = 1.0f;
    };


    class CoordinateMapperComponent
        : public AZ::Component
        , public Terrain::CoordinateMapperRequestBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(CoordinateMapperComponent, "{73750523-F6A9-4355-9912-BC134323CBBD}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        CoordinateMapperComponent(const CoordinateMapperConfig& configuration);
        CoordinateMapperComponent() = default;
        ~CoordinateMapperComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        void ConvertWorldAabbToTileNums(
            const AZ::Aabb& worldAabb, int zoomLevel, float& topTile, float& leftTile, float& bottomTile, float& rightTile) override;

        AZ::Vector2 GetMinMaxWorldHeights() override;

    private:
        CoordinateMapperConfig m_configuration;

        void LatLongToSlippyTile(float latitudeDegrees, float longitudeDegrees, int zoom, float& xTile, float& yTile);
    };
}
