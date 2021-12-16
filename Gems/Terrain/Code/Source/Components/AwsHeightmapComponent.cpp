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

#include <Components/AwsHeightmapComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Atom/Utils/PngFile.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Terrain
{
    void AwsHeightmapConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AwsHeightmapConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("EnableRefresh", &AwsHeightmapConfig::m_enableRefresh)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AwsHeightmapConfig>(
                    "Aws Elevation Map Gradient Component", "Provide heightmap data as a gradient directly from Aws Satellite Data")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AwsHeightmapConfig::m_enableRefresh, "Enable Refresh", "")
                    ;
            }
        }
    }

    void AwsHeightmapComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void AwsHeightmapComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void AwsHeightmapComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("ShapeService"));
    }

    void AwsHeightmapComponent::Reflect(AZ::ReflectContext* context)
    {
        AwsHeightmapConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AwsHeightmapComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AwsHeightmapComponent::m_configuration)
                ;
        }
    }

    AwsHeightmapComponent::AwsHeightmapComponent(const AwsHeightmapConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AwsHeightmapComponent::Activate()
    {
        CoordinateMapperNotificationBus::Handler::BusConnect();

        if (m_configuration.m_enableRefresh)
        {
            RefreshMinMaxHeights();
            OnImportTerrainTiles();

            AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

            GradientSignal::GradientRequestBus::Handler::BusConnect(GetEntityId());
        }
    }

    void AwsHeightmapComponent::Deactivate()
    {
        GradientSignal::GradientRequestBus::Handler::BusDisconnect();

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        CoordinateMapperNotificationBus::Handler::BusDisconnect();
    }

    bool AwsHeightmapComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AwsHeightmapConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AwsHeightmapComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AwsHeightmapConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }


    void AwsHeightmapComponent::OnImportTerrainTiles()
    {
        if (!m_cachedShapeBounds.IsValid())
        {
            return;
        }

        // This component uses https://registry.opendata.aws/terrain-tiles/ as a way to download real-world height data
        // directly into o3de terrain.

        // AWS Terrarium format tiles only come in 256x256 tile sizes:
        // https://github.com/tilezen/joerd/blob/master/docs/use-service.md
        const int tileSize = 256;
        // According to TileZen docs, zoom goes from 0-20, but 15 is the highest value with unique data.
        // https://github.com/tilezen/joerd/blob/master/docs/use-service.md
        const int zoom = 15;

        const int maxHeightmapSize = 4096;

        // Clamp to a max of 4k x 4k pixels by controlling the number of tiles we load in our grid in each direction.
        const int maxTilesToLoad = maxHeightmapSize / tileSize;

        float xTileLeft = 0.0f, yTileTop = 0.0f;
        float xTileRight = 0.0f, yTileBottom = 0.0f;

        CoordinateMapperRequestBus::Broadcast(
            &CoordinateMapperRequestBus::Events::ConvertWorldAabbToTileNums,
            m_cachedShapeBounds, zoom, yTileTop, xTileLeft, yTileBottom, xTileRight);

        if (((xTileRight - xTileLeft) <= 0.0f) && ((yTileBottom - yTileTop) <= 0.0f))
        {
            return;
        }

        // Clamp to a max of 4k x 4k pixels by controlling the number of tiles we load in our grid in each direction.
        xTileRight = AZStd::GetMin(xTileLeft + maxTilesToLoad, xTileRight);
        yTileBottom = AZStd::GetMin(yTileTop + maxTilesToLoad, yTileBottom);

        float xTileLeftInt, xTileLeftFrac, xTileRightInt, xTileRightFrac;
        float yTileTopInt, yTileTopFrac, yTileBottomInt, yTileBottomFrac;

        xTileLeftFrac = modf(xTileLeft, &xTileLeftInt);
        xTileRightFrac = modf(xTileRight, &xTileRightInt);
        yTileTopFrac = modf(yTileTop, &yTileTopInt);
        yTileBottomFrac = modf(yTileBottom, &yTileBottomInt);

        // Create the temp heightmap buffer to store all the tile data in
        m_heightmapData.clear();
        m_rawHeightmapHeight = static_cast<int>((yTileBottomInt - yTileTopInt + 1) * tileSize);
        m_rawHeightmapWidth = static_cast<int>((xTileRightInt - xTileLeftInt + 1) * tileSize);
        m_heightmapData.resize(m_rawHeightmapHeight * m_rawHeightmapWidth);

        // Download the tiles and copy them into the right place in the stitched image
        AZStd::string url;

        m_heightmapMinHeight = 32768.0f;
        m_heightmapMaxHeight = -32768.0f;

        AZ::JobCompletion jobCompletion;

        for (int yTile = static_cast<int>(yTileTopInt); yTile <= static_cast<int>(yTileBottomInt); yTile++)
        {
            for (int xTile = static_cast<int>(xTileLeftInt); xTile <= static_cast<int>(xTileRightInt); xTile++)
            {
                url = AZStd::string::format("https://s3.amazonaws.com/elevation-tiles-prod/terrarium/%d/%d/%d.png", zoom, xTile, yTile);
                AZ::Job* job = DownloadAndStitchTerrainTile(url, 0, 0, static_cast<int>((xTile - xTileLeftInt) * tileSize), static_cast<int>((yTile - yTileTopInt) * tileSize));

                job->SetDependent(&jobCompletion);
                job->Start();

                // TODO:  error handling!!!
            }
        }

        // TODO:  Change this to let the jobs run asynchronously, and just set terrain heightfield to dirty when they're all done.
        jobCompletion.StartAndWaitForCompletion();

        m_heightmapLeft = aznumeric_cast<uint32_t>(xTileLeftFrac * tileSize);
        m_heightmapTop = aznumeric_cast<uint32_t>(yTileTopFrac * tileSize);
        m_heightmapWidth = aznumeric_cast<uint32_t>((xTileRight - xTileLeft) * tileSize);
        m_heightmapHeight = aznumeric_cast<uint32_t>((yTileBottom - yTileTop) * tileSize);

        AZ::Vector2 minMaxHeights(0.0f);
        CoordinateMapperRequestBus::BroadcastResult(minMaxHeights, &CoordinateMapperRequestBus::Events::GetMinMaxWorldHeights);
        AZ_Assert(
            (minMaxHeights.GetX() <= m_heightmapMinHeight) && (minMaxHeights.GetY() >= m_heightmapMaxHeight),
            "Real-world data is outside the bounds of the Coordinate Mapper World Scale.  World Scale: (%.3f, %.3f).  Region Heights: "
            "(%.3f, %.3f)",
            minMaxHeights.GetX(), minMaxHeights.GetY(), m_heightmapMinHeight, m_heightmapMaxHeight);
        m_heightmapMinHeight = minMaxHeights.GetX();
        m_heightmapMaxHeight = minMaxHeights.GetY();
    }


    AZ::Job* AwsHeightmapComponent::DownloadAndStitchTerrainTile(const AZStd::string& url, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY)
    {
        AZ::JobContext* jobContext{ nullptr };
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction([=]()
        {
            std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

            Aws::String requestURL{ url.c_str() };
            auto httpRequest(Aws::Http::CreateHttpRequest(requestURL, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

            auto httpResponse(httpClient->MakeRequest(httpRequest, nullptr, nullptr));
            if (!httpResponse)
            {
                AZ_Error("Terrain", false, "Failed to download url: %s", url.c_str());
                //EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnError, "No Response Received from request!  (Internal SDK Error)");
                return;
            }

            int responseCode = static_cast<int>(httpResponse->GetResponseCode());
            if (responseCode != static_cast<int>(Aws::Http::HttpResponseCode::OK))
            {
                AZ_Error("Terrain", false, "Failed to download url: %s (Response code %d)", url.c_str(), responseCode);
                return;
            }

            AZStd::string contentType = httpResponse->GetContentType().c_str();

            AZStd::string returnString;
            auto& body = httpResponse->GetResponseBody();

            ProcessTerrainTile(body, tileStartX, tileStartY, stitchStartX, stitchStartY);

            // Debug code to save the downloaded PNG
            /*
            {
                std::ofstream fout(debugFileName.c_str(), std::ios::out | std::ios::binary);
                body.clear();
                body.seekg(0, std::ios::beg);
                fout << body.rdbuf();
                fout.close();
            }
            */

            AZ_TracePrintf("Terrain", "Successfully downloaded url: %s", url.c_str());

            //EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnSuccess, AZStd::string("Success!"));
            //EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, GetResponse, responseCode, stdHeaderMap, contentType, returnString);

        }, true, jobContext);
        return job;
    }

    void AwsHeightmapComponent::ProcessTerrainTile(Aws::IOStream& responseBody, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY)
    {
        AZStd::vector<uint8_t> pngBuffer;
        uint32_t pngWidth = 0;
        uint32_t pngHeight = 0;

        // Copy the PNG data out of our response into a memory buffer
        AZStd::copy(std::istreambuf_iterator<char>(responseBody), std::istreambuf_iterator<char>(), AZStd::back_inserter(pngBuffer));

        AZ::Utils::PngFile pngFile = AZ::Utils::PngFile::LoadFromBuffer(pngBuffer);
        pngWidth = pngFile.GetWidth();
        pngHeight = pngFile.GetHeight();
        auto pixelBuffer = pngFile.GetBuffer();
        auto pixelBufferFormat = pngFile.GetBufferFormat();


        AZ_Assert(pixelBufferFormat == AZ::Utils::PngFile::Format::RGB, "Unexpected pixel buffer format: %d.", pixelBufferFormat);
        AZ_Assert(pixelBuffer.size() == (pngWidth * pngHeight * 3),
            "Unexpected pixel buffer size: %d (expected %d * %d * 3)", pixelBuffer.size(), pngWidth, pngHeight);

        {
            float minHeight = 32768.0f;
            float maxHeight = -32768.0f;

            int srcHeight = pngHeight;
            int srcWidth = pngWidth;

            int dstHeight = m_rawHeightmapHeight;
            int dstWidth = m_rawHeightmapWidth;

            int copyWidth = AZStd::GetMin(srcWidth - tileStartX, dstWidth - stitchStartX);
            int copyHeight = AZStd::GetMin(srcHeight - tileStartY, dstHeight - stitchStartY);

            for (int height = 0; height < copyHeight; height++)
            {
                for (int width = 0; width < copyWidth; width++)
                {
                    const int bytesPerPixel = 3;
                    uint8_t* curSrcPixel = &pixelBuffer[(((height + tileStartY) * srcWidth) + width + tileStartX) * bytesPerPixel];

                    // The terrarium format stores heights in a fixed-point 16.8 format in RGB.  The conversion formula is the following:
                    // (R*256) + G + (B/256) - 32768
                    // This gives a range of -32768 to 32768 meters, at 1/256 m (~4 mm) precision.
                    float terrainHeight =
                          (float)((curSrcPixel[0] & 0xFF) * 256.0f)   // red
                        + (float) (curSrcPixel[1] & 0xFF)             // green
                        + (float)((curSrcPixel[2] & 0xFF) / 256.0f)   // blue
                        - 32768.0f;

                    // When storing in the image, we can either make (0,0) represent the "top left" of the data we downloaded, or
                    // the "bottom left".  We'll choose to make it the bottom left, so the Y value here gets flipped when writing
                    // into the image.
                    int dstX = width + stitchStartX;
                    int dstY = (dstHeight - 1) - (height + stitchStartY);
                    m_heightmapData[(dstY * dstWidth) + dstX] = terrainHeight;
                    minHeight = AZStd::GetMin(minHeight, terrainHeight);
                    maxHeight = AZStd::GetMax(maxHeight, terrainHeight);
                }
            }

            m_heightmapMinHeight = AZStd::GetMin(m_heightmapMinHeight, minHeight);
            m_heightmapMaxHeight = AZStd::GetMax(m_heightmapMaxHeight, maxHeight);

        }
    }

    float AwsHeightmapComponent::GetValue(const GradientSignal::GradientSampleParams& sampleParams) const
    {
        const float x = sampleParams.m_position.GetX();
        const float y = sampleParams.m_position.GetY();
        float height = 0.0f;
        if ((x >= m_cachedShapeBounds.GetMin().GetX()) && (x <= m_cachedShapeBounds.GetMax().GetX()) &&
            (y >= m_cachedShapeBounds.GetMin().GetY()) && (y <= m_cachedShapeBounds.GetMax().GetY()))
        {
            height = GetBilinearZ(x, y);
        }

        float scaledHeight = AZ::LerpInverse(m_heightmapMinHeight, m_heightmapMaxHeight, height);
        static bool printValues = false;
        if (printValues)
        {
            AZ_TracePrintf("Terrain", "%.2f, %.2f:  %.4f -> %.4f\n", x, y, height, scaledHeight);
        }
        return scaledHeight;
    }

    float AwsHeightmapComponent::GetBilinearZ(float x, float y) const
    {
        x -= m_cachedShapeBounds.GetMin().GetX();
        y -= m_cachedShapeBounds.GetMin().GetY();

        // Determine how to scale from our downloaded data to our existing terrain size
        const float xScale = m_heightmapWidth / static_cast<float>(m_cachedShapeBounds.GetXExtent());
        const float yScale = m_heightmapHeight / static_cast<float>(m_cachedShapeBounds.GetYExtent());

        // This method uses a 2x2 sampling kernel to get an interpolated height value.
        float srcHeights[2][2];

        // For each pixel in our destination terrain heightmap, use bilinear filtering to get
        // the appropriate source height value from our downloaded data.

        // Index of the downloaded pixel to lookup.
        int xLookup = static_cast<int>(x * xScale);
        int yLookup = static_cast<int>(y * yScale);

        // If our lookup pixel is outside the bounds of our downloaded heightmap, return 0.
        if ((xLookup < 0) || (yLookup < 0) || (xLookup >= m_heightmapWidth) || (yLookup >= m_heightmapHeight))
        {
            return 0.0f;
        }

        // How much the pixel index falls between adjacent downloaded pixels 
        // (used for our bilinear interpolation)
        float xLerp = (x * xScale) - xLookup;
        float yLerp = (y * yScale) - yLookup;

        // Sample our pixels.
        for (int ySample = 0; ySample < 2; ySample++)
        {
            for (int xSample = 0; xSample < 2; xSample++)
            {
                // Make sure to clamp our lookup values so that our bilinear filter doesn't run off the end of the data.
                const int xSampleLookup = AZStd::GetMin(xLookup + xSample, m_heightmapWidth - 1);
                const int ySampleLookup = AZStd::GetMin(yLookup + ySample, m_heightmapHeight - 1);

                srcHeights[xSample][ySample] = m_heightmapData[((ySampleLookup + m_heightmapTop) * m_rawHeightmapWidth) + (xSampleLookup + m_heightmapLeft)];
            }
        }

        // lerp between left and right for the top pixels
        float topHeight = AZ::Lerp(srcHeights[0][0], srcHeights[1][0], xLerp);
        // lerp between left and right for the bottom pixels
        float bottomHeight = AZ::Lerp(srcHeights[0][1], srcHeights[1][1], xLerp);
        // lerp bewteen the lerped top and bottom pixels
        float finalHeight = AZ::Lerp(topHeight, bottomHeight, yLerp);

        return finalHeight;
    }

    void AwsHeightmapComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /* world*/)
    {
        RefreshMinMaxHeights();
        OnImportTerrainTiles();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void AwsHeightmapComponent::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        RefreshMinMaxHeights();
        OnImportTerrainTiles();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void AwsHeightmapComponent::OnCoordinateMappingsChanged()
    {
        if (m_configuration.m_enableRefresh)
        {
            OnImportTerrainTiles();

            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    void AwsHeightmapComponent::RefreshMinMaxHeights()
    {
        if (m_configuration.m_enableRefresh)
        {
            // Get the height range of our height provider based on the shape component.
            LmbrCentral::ShapeComponentRequestsBus::EventResult(m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        }
    }
}
