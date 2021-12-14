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
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>

AZ_PUSH_DISABLE_WARNING(4251 4355 4996, "-Wunknown-warning-option")
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainMapboxMacroMaterialConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainMapboxMacroMaterialConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainMapboxMacroMaterialConfig, "{046BD6E2-3D9F-4E69-9445-5A96A17D377E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        float m_topLatitude = { 30.428292f };
        float m_leftLongitude = { -97.930687f };
        float m_bottomLatitude = { 30.381758f };
        float m_rightLongitude = { -97.857713f };
        bool m_enableRefresh = false;
        AZStd::string m_mapboxApiKey;
    };

    class TerrainMapboxMacroMaterialComponent
        : public AZ::Component
        , public TerrainMacroMaterialRequestBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainMapboxMacroMaterialComponent, "{59A730DD-3A2F-45FC-BD57-660E1C081F7A}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainMapboxMacroMaterialComponent(const TerrainMapboxMacroMaterialConfig& configuration);
        TerrainMapboxMacroMaterialComponent() = default;
        ~TerrainMapboxMacroMaterialComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        MacroMaterialData GetTerrainMacroMaterialData() override;

    private:
        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        void LatLongToTerrainTile(float latitudeDegrees, float longitudeDegrees, int zoom, float& xTile, float& yTile);
        void TerrainTileToLatLong(float xTile, float yTile, int zoom, float& latitudeDegrees, float& longitudeDegrees);

        void HandleMaterialStateChange();

        TerrainMapboxMacroMaterialConfig m_configuration;
        AZ::Aabb m_cachedShapeBounds;
        bool m_macroMaterialActive{ false };
        AZ::Data::Instance<AZ::RPI::AttachmentImage> m_downloadedImage;

        AZStd::vector<uint32_t> m_cachedPixels;
        int m_cachedPixelsHeight = 0;
        int m_cachedPixelsWidth = 0;

        void DownloadSatelliteImage();
        AZ::Job* DownloadAndStitchSatelliteImage(const AZStd::string& url, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);
        void ProcessSatelliteImage(Aws::IOStream& responseBody, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);
    };
}
