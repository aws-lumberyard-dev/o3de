/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

AZ_PUSH_DISABLE_WARNING(4251 4355 4996, "-Wunknown-warning-option")
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/Math/Aabb.h>

#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>

#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>



// This component uses https://registry.opendata.aws/terrain-tiles/ as a way to download real-world height data
// directly into Lumberyard terrain.

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class AwsHeightmapConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AwsHeightmapConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(AwsHeightmapConfig, "{1ABD721A-4957-4F64-91AA-2B362B098BE7}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        float m_topLatitude = { 30.428292f };
        float m_leftLongitude = { -97.930687f };
        float m_bottomLatitude = { 30.381758f };
        float m_rightLongitude = { -97.857713f };
        //float m_topLatitude = { 20.2705f };
        //float m_leftLongitude = { -155.9313f };
        //float m_bottomLatitude = { 19.9623f };
        //float m_rightLongitude = { -155.5619f };
        bool m_enableRefresh = false;
    };


    class AwsHeightmapComponent
        : public AZ::Component
        , private GradientSignal::GradientRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(AwsHeightmapComponent, "{7874B759-0BFC-4FC7-9419-051747137771}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AwsHeightmapComponent(const AwsHeightmapConfig& configuration);
        AwsHeightmapComponent() = default;
        ~AwsHeightmapComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override;


        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

    private:
        float GetBilinearZ(float x, float y) const;

        void RefreshMinMaxHeights();

        void OnImportTerrainTiles();

        void LatLongToTerrainTile(float latitudeDegrees, float longitudeDegrees, int zoom, float& xTile, float& yTile);
        void TerrainTileToLatLong(float xTile, float yTile, int zoom, float& latitudeDegrees, float& longitudeDegrees);

        AZ::Job* DownloadAndStitchTerrainTile(const AZStd::string& url, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);
        void ProcessTerrainTile(Aws::IOStream& responseBody, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);

        AwsHeightmapConfig m_configuration;

        AZStd::vector<float> m_heightmapData;
        int m_heightmapHeight = 0;
        int m_heightmapWidth = 0;
        float m_heightmapMinHeight = 0.0f;
        float m_heightmapMaxHeight = 0.0f;

        AZ::Aabb m_cachedShapeBounds;
        bool m_refreshHeightData = true;
    };
}
