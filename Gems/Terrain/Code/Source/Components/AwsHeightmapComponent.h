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

#include <Terrain/Ebuses/CoordinateMapperRequestBus.h>


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

        bool m_enableRefresh = false;
    };


    class AwsHeightmapComponent
        : public AZ::Component
        , private GradientSignal::GradientRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private CoordinateMapperNotificationBus::Handler
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
        // CoordinateMapperNotificationBus
        void OnCoordinateMappingsChanged() override;

        float GetBilinearZ(float x, float y) const;

        void RefreshMinMaxHeights();

        void OnImportTerrainTiles();

        AZ::Job* DownloadAndStitchTerrainTile(const AZStd::string& url, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);
        void ProcessTerrainTile(Aws::IOStream& responseBody, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY);

        AwsHeightmapConfig m_configuration;

        // This buffer contains full tile-sized chunks of data, but we generally need only partial tiles on the boundaries.
        // We'll use offsets to control the valid usable window of data inside this buffer.
        AZStd::vector<float> m_heightmapData;

        // This is the size of the full buffer above.
        int m_rawHeightmapHeight = 0;
        int m_rawHeightmapWidth = 0;

        // These are the min/max height values found in the data, used for auto-scaling.
        float m_heightmapMinHeight = 0.0f;
        float m_heightmapMaxHeight = 0.0f;

        AZ::Aabb m_cachedShapeBounds;

        // Window of usable values within the raw heightmap data buffer.
        int m_heightmapLeft = 0;
        int m_heightmapTop = 0;
        int m_heightmapWidth = 0;
        int m_heightmapHeight = 0;

    };
}
