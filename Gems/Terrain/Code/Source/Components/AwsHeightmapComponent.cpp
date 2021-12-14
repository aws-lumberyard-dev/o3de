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

#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <Atom/Utils/PngFile.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>


namespace Terrain
{
    void AwsHeightmapConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AwsHeightmapConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("TopLatitude", &AwsHeightmapConfig::m_topLatitude)
                ->Field("LeftLongitude", &AwsHeightmapConfig::m_leftLongitude)
                ->Field("BottomLatitude", &AwsHeightmapConfig::m_bottomLatitude)
                ->Field("RightLongitude", &AwsHeightmapConfig::m_rightLongitude)
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

                    ->DataElement(AZ::Edit::UIHandlers::Default, &AwsHeightmapConfig::m_topLatitude, "Top Latitude", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AwsHeightmapConfig::m_leftLongitude, "Left Longitude", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AwsHeightmapConfig::m_bottomLatitude, "Bottom Latitude", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AwsHeightmapConfig::m_rightLongitude, "Right Longitude", "")
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

            /*
            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAwsHeightmapComponent>(
                    "Aws Heightmap", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                    ->UIElement(AZ::Edit::UIHandlers::Button, "DownloadHeightmap", "Download heightmap data")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Download Height Data")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAwsHeightmapComponent::OnImportTerrainTiles)
                    ;
            }
            */
        }
    }

    AwsHeightmapComponent::AwsHeightmapComponent(const AwsHeightmapConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AwsHeightmapComponent::Activate()
    {
        if (m_configuration.m_enableRefresh)
        {
            m_refreshHeightData = true;
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
        // This component uses https://registry.opendata.aws/terrain-tiles/ as a way to download real-world height data
        // directly into Lumberyard terrain.
        // Use https://www.openstreetmap.org/export#map=15/30.4019/-97.8937 as a way to get lat / long values


        // AWS Terrarium format tiles only come in 256x256 tile sizes:
        // https://github.com/tilezen/joerd/blob/master/docs/use-service.md
        const int tileSize = 256;
        // According to TileZen docs, zoom goes from 0-20, but 15 is the highest value with unique data.
        // https://github.com/tilezen/joerd/blob/master/docs/use-service.md
        const int zoom = 15;
        const int maxHeightmapSize = 4096;

        float top = m_configuration.m_topLatitude;
        float left = m_configuration.m_leftLongitude;
        float bottom = m_configuration.m_bottomLatitude;
        float right = m_configuration.m_rightLongitude;

        float xTileLeft = 0.0f, yTileTop = 0.0f;
        float xTileRight = 0.0f, yTileBottom = 0.0f;

        // Based on the lat / long coordinates and zoom level, get the top left tile XY name.
        LatLongToTerrainTile(top, left, zoom, xTileLeft, yTileTop);

        // This would default to the correct right/bottom values for a heightmap of a given size.
        //xTileRight = xTileLeft + (m_pHeightmap->GetWidth() / tileSize) - 1;
        //yTileBottom = yTileTop + (m_pHeightmap->GetHeight() / tileSize) - 1;
        //TerrainTileToLatLong(xTileRight, yTileBottom, zoom, bottom, right);

        // Based on the lat / long coordinates and zoom level, get the bottom right tile XY name.
        LatLongToTerrainTile(bottom, right, zoom, xTileRight, yTileBottom);

        // Clamp to a max of 4k x 4k pixels by controlling the number of tiles we load in our grid in each direction.
        xTileRight = AZStd::GetMin((xTileLeft + (maxHeightmapSize / tileSize)), xTileRight);
        yTileBottom = AZStd::GetMin((yTileTop + (maxHeightmapSize / tileSize)), yTileBottom);

        // Calculate the heightmap XY resolution in the data we've downloaded.  This isn't needed for the component to run,
        // but it's useful to understand the max quality level we'll get from this data.
        // (Math found here - http://wiki.openstreetmap.org/wiki/Zoom_levels )
        const float equatorCircumferenceMeters = 40075017;
        const float tileMetersPerPixel = (equatorCircumferenceMeters * cos(AZ::DegToRad(top)) / pow(2.0f, zoom + 8.0f));
        AZ_TracePrintf("Terrain", "The terrain tile resolution has %.3f meters per pixel.", tileMetersPerPixel);

        //TODO: Account for fractional offsets.  Right now, we force things to 256x256 tile boundaries.
        xTileLeft = floorf(xTileLeft);
        xTileRight = floorf(xTileRight);
        yTileTop = floorf(yTileTop);
        yTileBottom = floorf(yTileBottom);

        // Create the temp heightmap buffer to store all the tile data in
        m_heightmapData.clear();
        m_heightmapHeight = static_cast<int>((yTileBottom - yTileTop + 1) * tileSize);
        m_heightmapWidth = static_cast<int>((xTileRight - xTileLeft + 1) * tileSize);
        m_heightmapData.resize(m_heightmapHeight * m_heightmapWidth);

        // Download the tiles and copy them into the right place in the stitched image
        AZStd::string url;

        m_heightmapMinHeight = 32768.0f;
        m_heightmapMaxHeight = -32768.0f;

        AZ::JobCompletion jobCompletion;

        for (int yTile = static_cast<int>(yTileTop); yTile <= static_cast<int>(yTileBottom); yTile++)
        {
            for (int xTile = static_cast<int>(xTileLeft); xTile <= static_cast<int>(xTileRight); xTile++)
            {
                url = AZStd::string::format("https://s3.amazonaws.com/elevation-tiles-prod/terrarium/%d/%d/%d.png", zoom, xTile, yTile);
                AZ::Job* job = DownloadAndStitchTerrainTile(url, 0, 0, static_cast<int>((xTile - xTileLeft) * tileSize), static_cast<int>((yTile - yTileTop) * tileSize));

                job->SetDependent(&jobCompletion);
                job->Start();

                // TODO:  error handling!!!
            }
        }

        // TODO:  Change this to let the jobs run asynchronously, and just set terrain heightfield to dirty when they're all done.
        jobCompletion.StartAndWaitForCompletion();
    }

    void AwsHeightmapComponent::LatLongToTerrainTile(float latitudeDegrees, float longitudeDegrees, int zoom, float& xTile, float& yTile)
    {
        // Tile calculation math found here - http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames 
        float latitudeRadians = AZ::DegToRad(latitudeDegrees);

        double n = pow(2.0f, zoom);
        xTile = static_cast<float>(n * ((longitudeDegrees + 180.0f) / 360.0f));
        yTile = static_cast<float>(n * (1.0f - (log(tan(latitudeRadians) + (1.0f / cos(latitudeRadians))) / AZ::Constants::Pi)) / 2.0f);
    }

    void AwsHeightmapComponent::TerrainTileToLatLong(float xTile, float yTile, int zoom, float& latitudeDegrees, float& longitudeDegrees)
    {
        // Tile calculation math found here - http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames 
        double n = pow(2.0f, zoom);
        longitudeDegrees = static_cast<float>(xTile / n * 360.0f - 180.0f);
        float latitudeRadians = static_cast<float>(atan(sinh(AZ::Constants::Pi * (1.0f - 2.0f * yTile / n))));
        latitudeDegrees = AZ::RadToDeg(latitudeRadians);
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

            int dstHeight = m_heightmapHeight;
            int dstWidth = m_heightmapWidth;

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

                srcHeights[xSample][ySample] = m_heightmapData[(ySampleLookup * m_heightmapWidth) + xSampleLookup];
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
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
    }

    void AwsHeightmapComponent::OnShapeChanged(ShapeChangeReasons /*changeReason*/)
    {
        m_refreshHeightData = true;
        RefreshMinMaxHeights();
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

