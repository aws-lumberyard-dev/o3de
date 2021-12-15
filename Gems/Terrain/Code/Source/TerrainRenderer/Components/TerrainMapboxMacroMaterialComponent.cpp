/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainMapboxMacroMaterialComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/Utils/PngFile.h>

#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <AzCore/IO/SystemFile.h>

#include <Terrain/Ebuses/CoordinateMapperRequestBus.h>

namespace Terrain
{
    void TerrainMapboxMacroMaterialConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context); serialize)
        {
            serialize->Class<TerrainMapboxMacroMaterialConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("EnableRefresh", &TerrainMapboxMacroMaterialConfig::m_enableRefresh)
                ->Field("ApiKey", &TerrainMapboxMacroMaterialConfig::m_mapboxApiKey)
                ;

            if (auto* editContext = serialize->GetEditContext(); editContext)
            {
                editContext
                    ->Class<TerrainMapboxMacroMaterialConfig>(
                        "Terrain Macro Material Component", "Provide a terrain macro material for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &TerrainMapboxMacroMaterialConfig::m_enableRefresh, "Enable Refresh", "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMapboxMacroMaterialConfig::m_mapboxApiKey, "API Key",
                        "The Mapbox API key to use.")
                    ;
            }
        }
    }

    void TerrainMapboxMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMapboxMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMapboxMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainMapboxMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainMapboxMacroMaterialConfig::Reflect(context);
        
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainMapboxMacroMaterialComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainMapboxMacroMaterialComponent::m_configuration)
            ;
        }
    }

    TerrainMapboxMacroMaterialComponent::TerrainMapboxMacroMaterialComponent(const TerrainMapboxMacroMaterialConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainMapboxMacroMaterialComponent::Activate()
    {
        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Clear out our shape bounds.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        if (m_configuration.m_enableRefresh)
        {
            DownloadSatelliteImage();
        }
    }

    void TerrainMapboxMacroMaterialComponent::Deactivate()
    {
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        m_downloadedImage.reset();
        m_cachedPixels.clear();

        // Send out any notifications as appropriate based on the macro material destruction.
        HandleMaterialStateChange();
    }

    bool TerrainMapboxMacroMaterialComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainMapboxMacroMaterialConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainMapboxMacroMaterialComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainMapboxMacroMaterialConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainMapboxMacroMaterialComponent::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        // This should only get called while the macro material is active.  If it gets called while the macro material isn't active,
        // we've got a bug where we haven't managed the bus connections properly.
        AZ_Assert(m_macroMaterialActive, "The ShapeComponentNotificationBus connection is out of sync with the material load.");

        AZ::Aabb oldShapeBounds = m_cachedShapeBounds;

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialRegionChanged,
            GetEntityId(), oldShapeBounds, m_cachedShapeBounds);

        if (m_configuration.m_enableRefresh)
        {
            DownloadSatelliteImage();
        }
    }

    void TerrainMapboxMacroMaterialComponent::HandleMaterialStateChange()
    {
        // We only want our component to appear active during the time that the macro material is fully loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        // Color data is considered ready if it's finished loading
        bool colorReady = m_downloadedImage;

        bool wasPreviouslyActive = m_macroMaterialActive;
        bool isNowActive = colorReady;

        // Set our state to active or inactive, based on whether or not the macro material instance is now valid.
        m_macroMaterialActive = isNowActive;

        // Handle the different inactive/active transition possibilities.

        if (!wasPreviouslyActive && !isNowActive)
        {
            // Do nothing, we haven't yet successfully loaded a valid material.
        }
        else if (!wasPreviouslyActive && isNowActive)
        {
            // We've transitioned from inactive to active, so send out a message saying that we've been created and start tracking the
            // overall shape bounds.

            // Get the current shape bounds.
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            // Start listening for terrain macro material requests.
            TerrainMacroMaterialRequestBus::Handler::BusConnect(GetEntityId());

            // Start listening for shape changes.
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

            MacroMaterialData material = GetTerrainMacroMaterialData();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialCreated, GetEntityId(), material);
        }
        else if (wasPreviouslyActive && !isNowActive)
        {
            // Stop listening to macro material requests or shape changes, and send out a notification that we no longer have a valid
            // macro material.

            TerrainMacroMaterialRequestBus::Handler::BusDisconnect();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

            m_cachedShapeBounds = AZ::Aabb::CreateNull();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialDestroyed, GetEntityId());
        }
        else
        {
            // We were active both before and after, so just send out a material changed event.
            MacroMaterialData material = GetTerrainMacroMaterialData();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, GetEntityId(), material);
        }
    }

    MacroMaterialData TerrainMapboxMacroMaterialComponent::GetTerrainMacroMaterialData()
    {
        MacroMaterialData macroMaterial;

        macroMaterial.m_entityId = GetEntityId();
        macroMaterial.m_bounds = m_cachedShapeBounds;
        macroMaterial.m_colorImage = m_downloadedImage;

        return macroMaterial;
    }

    void TerrainMapboxMacroMaterialComponent::DownloadSatelliteImage()
    {
        if (!m_cachedShapeBounds.IsValid())
        {
            return;
        }

        // Mapbox raster images requested at 2x are 512 x 512 in size.
        // https://docs.mapbox.com/api/maps/raster-tiles/#example-request-retrieve-raster-tiles
        const int tileSize = 512;
        // Zoom goes from 0-20, but we'll use 15 to align with the terrain height data.
        // https://github.com/tilezen/joerd/blob/master/docs/use-service.md
        const int zoom = 15;
        const int maxCachedPixelSize = 8192;

        // Clamp to a max of 4k x 4k pixels by controlling the number of tiles we load in our grid in each direction.
        const int maxTilesToLoad = maxCachedPixelSize / tileSize;

        float xTileLeft = 0.0f, yTileTop = 0.0f;
        float xTileRight = 0.0f, yTileBottom = 0.0f;

        CoordinateMapperRequestBus::Broadcast(
            &CoordinateMapperRequestBus::Events::ConvertWorldAabbToTileNums, m_cachedShapeBounds, zoom, yTileTop, xTileLeft, yTileBottom,
            xTileRight);

        if (((xTileRight - xTileLeft) <= 0.0f) && ((yTileBottom - yTileTop) <= 0.0f))
        {
            return;
        }

        // Clamp to a max of 8k x 8k pixels by controlling the number of tiles we load in our grid in each direction.
        xTileRight = AZStd::GetMin(xTileLeft + maxTilesToLoad, xTileRight);
        yTileBottom = AZStd::GetMin(yTileTop + maxTilesToLoad, yTileBottom);

        float xTileLeftInt, xTileLeftFrac, xTileRightInt, xTileRightFrac;
        float yTileTopInt, yTileTopFrac, yTileBottomInt, yTileBottomFrac;

        xTileLeftFrac = modf(xTileLeft, &xTileLeftInt);
        xTileRightFrac = modf(xTileRight, &xTileRightInt);
        yTileTopFrac = modf(yTileTop, &yTileTopInt);
        yTileBottomFrac = modf(yTileBottom, &yTileBottomInt);

        // Create the temp pixel buffer to store all the tile data in
        m_cachedPixels.clear();
        m_cachedPixelsHeight = static_cast<int>((yTileBottomInt - yTileTopInt + 1) * tileSize);
        m_cachedPixelsWidth = static_cast<int>((xTileRightInt - xTileLeftInt + 1) * tileSize);
        m_cachedPixels.resize(m_cachedPixelsHeight * m_cachedPixelsWidth);

        uint32_t pixelWidth = aznumeric_cast<uint32_t>((xTileRight - xTileLeft) * tileSize);
        uint32_t pixelHeight = aznumeric_cast<uint32_t>((yTileBottom - yTileTop) * tileSize);

        // Because of the way our images are stored vs our coordinates, the X offset is from the left,
        // but the top Y offset needs to skip the bottom fraction of pixels, not the top fraction.
        uint32_t xPixelLeft = aznumeric_cast<uint32_t>(xTileLeftFrac * tileSize);
        uint32_t yPixelTop = (m_cachedPixelsHeight - pixelHeight) - aznumeric_cast<uint32_t>(yTileTopFrac * tileSize);

        // Create the initial buffer for the downloaded color data
        const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
            AZ::RHI::ImageBindFlags::ShaderRead, pixelWidth, pixelHeight, AZ::RHI::Format::R8G8B8A8_UNORM);

        const AZ::Name DownloadedImageName = AZ::Name("DownloadedImage");
        m_downloadedImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, DownloadedImageName, nullptr, nullptr);
        AZ_Error("Terrain", m_downloadedImage, "Failed to initialize the downloaded image buffer.");


        // Download the tiles and copy them into the right place in the stitched image
        AZStd::string url;

        AZ::JobCompletion jobCompletion;

        for (int yTile = static_cast<int>(yTileTopInt); yTile <= static_cast<int>(yTileBottomInt); yTile++)
        {
            for (int xTile = static_cast<int>(xTileLeftInt); xTile <= static_cast<int>(xTileRightInt); xTile++)
            {
                url = AZStd::string::format(
                    "https://api.mapbox.com/v4/mapbox.satellite/%d/%d/%d@2x.png256?access_token=%s",
                    zoom, xTile, yTile, m_configuration.m_mapboxApiKey.c_str());
                AZ::Job* job = DownloadAndStitchSatelliteImage(
                    url, 0, 0, static_cast<int>((xTile - xTileLeftInt) * tileSize), static_cast<int>((yTile - yTileTopInt) * tileSize));

                job->SetDependent(&jobCompletion);
                job->Start();

                // TODO:  error handling!!!
            }
        }

        // TODO:  Change this to let the jobs run asynchronously, and just set terrain heightfield to dirty when they're all done.
        jobCompletion.StartAndWaitForCompletion();

        constexpr uint32_t BytesPerPixel = sizeof(uint32_t);

        AZStd::vector<uint32_t> cachedPixelSubRegion;
        cachedPixelSubRegion.resize(pixelWidth * pixelHeight);
        for (uint32_t line = 0; line < pixelHeight; line++)
        {
            uint32_t* srcLineStart = &(m_cachedPixels[(line + yPixelTop) * m_cachedPixelsWidth]);
            uint32_t* subregionLineStart = &(cachedPixelSubRegion[line * pixelWidth]);
            memcpy(subregionLineStart, &(srcLineStart[xPixelLeft]), pixelWidth * BytesPerPixel);
        }

        AZ::RHI::ImageUpdateRequest imageUpdateRequest;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = 0;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = 0;
        imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = pixelWidth * BytesPerPixel;
        imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = pixelWidth * pixelHeight * BytesPerPixel;
        imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = pixelHeight;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = pixelWidth;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = pixelHeight;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_depth = 1;
        imageUpdateRequest.m_sourceData = cachedPixelSubRegion.data();
        imageUpdateRequest.m_image = m_downloadedImage->GetRHIImage();

        m_downloadedImage->UpdateImageContents(imageUpdateRequest);

        HandleMaterialStateChange();
    }


    AZ::Job* TerrainMapboxMacroMaterialComponent::DownloadAndStitchSatelliteImage(
        const AZStd::string& url, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY)
    {
        AZ::JobContext* jobContext{ nullptr };
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
        AZ::Job* job{ nullptr };
        job = AZ::CreateJobFunction(
            [=]()
            {
                std::shared_ptr<Aws::Http::HttpClient> httpClient = Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

                Aws::String requestURL{ url.c_str() };
                auto httpRequest(Aws::Http::CreateHttpRequest(
                    requestURL, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

                auto httpResponse(httpClient->MakeRequest(httpRequest, nullptr, nullptr));
                if (!httpResponse)
                {
                    AZ_Error("Terrain", false, "Failed to download url: %s", url.c_str());
                    // EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnError, "No Response Received from request!  (Internal SDK Error)");
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

                // Debug code to save the downloaded PNG
                /*
                {
                    AZStd::vector<char> pngBuffer;
                    AZ::IO::SystemFile debugFile;

                    AZStd::string debugFileName("D:\\downloadedImage.png");

                    debugFile.Open(
                        debugFileName.c_str(),
                        AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY);
                    // Copy the PNG data out of our response into a memory buffer
                    AZStd::copy(std::istreambuf_iterator<char>(body), std::istreambuf_iterator<char>(), AZStd::back_inserter(pngBuffer));
                    debugFile.Write(pngBuffer.data(), pngBuffer.size());
                    debugFile.Flush();
                    debugFile.Close();

                    body.clear();
                    body.seekg(0, std::ios::beg);
                }
                */

                ProcessSatelliteImage(body, tileStartX, tileStartY, stitchStartX, stitchStartY);

                AZ_TracePrintf("Terrain", "Successfully downloaded url: %s", url.c_str());

                // EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, OnSuccess, AZStd::string("Success!"));
                // EBUS_EVENT(AWSBehaviorHTTPNotificationsBus, GetResponse, responseCode, stdHeaderMap, contentType, returnString);
            },
            true, jobContext);
        return job;
    }

    void TerrainMapboxMacroMaterialComponent::ProcessSatelliteImage(
        Aws::IOStream& responseBody, int tileStartX, int tileStartY, int stitchStartX, int stitchStartY)
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

        {
            int srcHeight = pngHeight;
            int srcWidth = pngWidth;

            int dstHeight = m_cachedPixelsHeight;
            int dstWidth = m_cachedPixelsWidth;

            int copyWidth = AZStd::GetMin(srcWidth - tileStartX, dstWidth - stitchStartX);
            int copyHeight = AZStd::GetMin(srcHeight - tileStartY, dstHeight - stitchStartY);

            for (int height = 0; height < copyHeight; height++)
            {
                for (int width = 0; width < copyWidth; width++)
                {
                    uint8_t red = 0;
                    uint8_t green = 0;
                    uint8_t blue = 0;
                    uint8_t alpha = 0xFF;
                    switch (pixelBufferFormat)
                    {
                    case AZ::Utils::PngFile::Format::RGB:
                        {
                            const size_t bytesPerPixel = 3;
                            size_t curPixelOffset =
                                ((height + tileStartY) * srcWidth * bytesPerPixel) + ((width + tileStartX) * bytesPerPixel);
                            red = pixelBuffer[curPixelOffset];
                            green = pixelBuffer[curPixelOffset + 1];
                            blue = pixelBuffer[curPixelOffset + 2];
                        }
                        break;
                    case AZ::Utils::PngFile::Format::RGBA:
                        {
                            const size_t bytesPerPixel = 4;
                            size_t curPixelOffset =
                                ((height + tileStartY) * srcWidth * bytesPerPixel) + ((width + tileStartX) * bytesPerPixel);
                            red = pixelBuffer[curPixelOffset];
                            green = pixelBuffer[curPixelOffset + 1];
                            blue = pixelBuffer[curPixelOffset + 2];
                            alpha = pixelBuffer[curPixelOffset + 3];
                        }
                        break;
                    default:
                        AZ_Error("Terrain", false, "Unknown Png File buffer format.");
                        break;
                    }
                    uint32_t curSrcPixel = (alpha << 24) | (blue << 16) | (green << 8) | red;

                    int dstX = width + stitchStartX;
                    int dstY = height + stitchStartY;
                    m_cachedPixels[(dstY * dstWidth) + dstX] = curSrcPixel;
                }
            }
        }
    }

} // namespace Terrain

