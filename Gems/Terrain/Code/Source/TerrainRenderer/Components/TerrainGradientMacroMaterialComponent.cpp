/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainGradientMacroMaterialComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>

#include <AzCore/IO/SystemFile.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

#pragma optimize("", off)

namespace Terrain
{
    void TerrainGradientColorMapping::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainGradientColorMapping>()
                ->Version(1)
                ->Field("Color1", &TerrainGradientColorMapping::m_color1)
                ->Field("Color2", &TerrainGradientColorMapping::m_color2)
                ->Field("Mask Gradient", &TerrainGradientColorMapping::m_maskEntityId)
                ;

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainGradientColorMapping>("Terrain Gradient Color Mapping", "Gradient to color variation mapping.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientColorMapping::m_color1, "Color 1",
                        "First color to use in the blend.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientColorMapping::m_color2, "Color 2",
                        "Second color to use in the blend.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientColorMapping::m_maskEntityId, "Mask Gradient",
                        "Entity that provides a gradient that determines the strength of the color.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->UIElement("GradientPreviewer", "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC_CE("GradientEntity"), &TerrainGradientColorMapping::m_maskEntityId)
                    ;
            }
        }
    }

    void TerrainGradientMacroMaterialConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainGradientColorMapping::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context); serialize)
        {
            serialize->Class<TerrainGradientMacroMaterialConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Color Variation Gradient", &TerrainGradientMacroMaterialConfig::m_variationEntityId)
                ->Field("Image Resolution", &TerrainGradientMacroMaterialConfig::m_imageResolution)
                ->Field("Base Color 1", &TerrainGradientMacroMaterialConfig::m_baseColor1)
                ->Field("Base Color 2", &TerrainGradientMacroMaterialConfig::m_baseColor2)
                ->Field("Gradient Color Mappings", &TerrainGradientMacroMaterialConfig::m_gradientColorMappings)
                ;

            if (auto* editContext = serialize->GetEditContext(); editContext)
            {
                editContext
                    ->Class<TerrainGradientMacroMaterialConfig>(
                        "Terrain Gradient Material Component", "Generate a terrain macro material from a series of gradients")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->UIElement("GradientPreviewer", "Previewer")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->Attribute(AZ_CRC_CE("GradientEntity"), &TerrainGradientMacroMaterialConfig::m_variationEntityId)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientMacroMaterialConfig::m_variationEntityId, "Color Variation Gradient",
                        "Entity that provides a gradient that determines the blend between the two colors.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientMacroMaterialConfig::m_imageResolution,
                        "Image Resolution", "Number of pixels in the generated image.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &TerrainGradientMacroMaterialConfig::m_baseColor1,
                        "Default Color 1",
                        "First color to use in the blend.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &TerrainGradientMacroMaterialConfig::m_baseColor2,
                        "Default Color 2",
                        "Second color to use in the blend.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainGradientMacroMaterialConfig::m_gradientColorMappings,
                        "Gradient Color Mappings", "Mappings of colors, strengths, and blends.")
                    ;
            }
        }
    }

    void TerrainGradientMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainGradientMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainGradientMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainGradientMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainGradientMacroMaterialConfig::Reflect(context);
        
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainGradientMacroMaterialComponent, AZ::Component>()
                ->Version(0)->Field(
                "Configuration", &TerrainGradientMacroMaterialComponent::m_configuration)
            ;
        }
    }

    TerrainGradientMacroMaterialComponent::TerrainGradientMacroMaterialComponent(const TerrainGradientMacroMaterialConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainGradientMacroMaterialComponent::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Clear out our shape bounds.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Make sure we get update notifications whenever this entity or any dependent gradient entity changes in any way.
        // We'll use that to rebuild our generated macro material.
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        if (m_configuration.m_variationEntityId != GetEntityId())
        {
            m_dependencyMonitor.ConnectDependency(m_configuration.m_variationEntityId);
        }

        for (auto& mapping : m_configuration.m_gradientColorMappings)
        {
            if (mapping.m_maskEntityId != GetEntityId())
            {
                m_dependencyMonitor.ConnectDependency(mapping.m_maskEntityId);
            }
        }

        QueueRefresh();
    }

    void TerrainGradientMacroMaterialComponent::QueueRefresh()
    {
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void TerrainGradientMacroMaterialComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        GenerateMacroMaterial();

        AZ::TickBus::Handler::BusDisconnect();
    }


    void TerrainGradientMacroMaterialComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        m_generatedImage.reset();
        m_cachedPixels.clear();

        m_dependencyMonitor.Reset();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // Send out any notifications as appropriate based on the macro material destruction.
        HandleMaterialStateChange();
    }

    bool TerrainGradientMacroMaterialComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainGradientMacroMaterialConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainGradientMacroMaterialComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainGradientMacroMaterialConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainGradientMacroMaterialComponent::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
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

        QueueRefresh();
    }

    void TerrainGradientMacroMaterialComponent::HandleMaterialStateChange()
    {
        // We only want our component to appear active during the time that the macro material is fully loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        // Color data is considered ready if it's finished loading
        bool colorReady = m_generatedImage;

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

    MacroMaterialData TerrainGradientMacroMaterialComponent::GetTerrainMacroMaterialData()
    {
        MacroMaterialData macroMaterial;

        macroMaterial.m_entityId = GetEntityId();
        macroMaterial.m_bounds = m_cachedShapeBounds;
        macroMaterial.m_colorImage = m_generatedImage;

        return macroMaterial;
    }

    void TerrainGradientMacroMaterialComponent::GenerateMacroMaterial()
    {
        if (!m_cachedShapeBounds.IsValid())
        {
            return;
        }

        // Create a pixel buffer to hold our generated colors
        m_cachedPixels.clear();
        m_cachedPixelsHeight = aznumeric_cast<int>(m_configuration.m_imageResolution.GetY());
        m_cachedPixelsWidth = aznumeric_cast<int>(m_configuration.m_imageResolution.GetX());
        m_cachedPixels.resize(m_cachedPixelsHeight * m_cachedPixelsWidth);

        if ((m_cachedPixelsWidth == 0) || (m_cachedPixelsHeight == 0))
        {
            return;
        }

        // Create the render texture buffer for the generated color data
        const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
            AZ::RHI::ImageBindFlags::ShaderRead, m_cachedPixelsWidth, m_cachedPixelsHeight, AZ::RHI::Format::R8G8B8A8_UNORM_SRGB);

        const AZ::Name GeneratedImage = AZ::Name("GeneratedTerrainMacroColor");
        m_generatedImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, GeneratedImage, nullptr, nullptr);
        AZ_Error("Terrain", m_generatedImage, "Failed to initialize the generated image buffer.");

        AZ::JobContext* jobContext{ nullptr };
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);

        // Generate the world locations of every pixel in our generated image to use in our gradient queries.
        AZ::Job* generatePixelDataJob = AZ::CreateJobFunction(
            [=]()
            {
                AZStd::vector<AZ::Vector3> gradientQueryPositions;
                gradientQueryPositions.reserve(m_cachedPixelsWidth * m_cachedPixelsHeight);

                AZ::Vector2 stepSize(
                    m_cachedShapeBounds.GetXExtent() / m_cachedPixelsWidth, m_cachedShapeBounds.GetYExtent() / m_cachedPixelsHeight);

                // The order of our position list will determine the order in which we fill in pixels in our cached pixel structure.
                // We invert the y loop because we have a convention in which we want the bottom of the texture to be aligned with the
                // bottom of world space so that looking in the +x +y direction from 0,0 shows the texture going in the direction we expect.
                for (int y = m_cachedPixelsHeight; y > 0; y--)
                {
                    for (int x = 0; x < m_cachedPixelsWidth; x++)
                    {
                        float worldX = m_cachedShapeBounds.GetMin().GetX() + (aznumeric_cast<float>(x) * stepSize.GetX());
                        float worldY = m_cachedShapeBounds.GetMin().GetY() + (aznumeric_cast<float>(y) * stepSize.GetY());

                        gradientQueryPositions.emplace_back(worldX, worldY, m_cachedShapeBounds.GetCenter().GetZ());
                    }
                }

                // Block other threads from accessing the surface data bus while we are in GetValue (which may call into the SurfaceData
                // bus). This prevents lock inversion deadlocks between this calling Gradient->Surface and something else calling
                // Surface->Gradient.
                auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
                typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

                // Get each set of gradient values and use that to create the color to alpha blend into our texture.

                AZStd::vector<float> variationValues(gradientQueryPositions.size());
                GradientSignal::GradientRequestBus::Event(
                    m_configuration.m_variationEntityId, &GradientSignal::GradientRequestBus::Events::GetValues, gradientQueryPositions,
                    variationValues);

                for (uint32_t index = 0; index < m_cachedPixels.size(); index++)
                {
                    uint32_t& srcPixel = m_cachedPixels[index];
                    AZ::Color pixel;
                    pixel.FromU32(srcPixel);

                    // Create our default color variation and store it in our cached pixel buffer.
                    AZ::Color destPixel = m_configuration.m_baseColor1;
                    srcPixel = destPixel.Lerp(m_configuration.m_baseColor2, variationValues[index]).ToU32();
                }

                AZStd::vector<float> maskValues(gradientQueryPositions.size());

                for (auto& mapping : m_configuration.m_gradientColorMappings)
                {
                    GradientSignal::GradientRequestBus::Event(
                        mapping.m_maskEntityId, &GradientSignal::GradientRequestBus::Events::GetValues, gradientQueryPositions, maskValues);

                    for (uint32_t index = 0; index < m_cachedPixels.size(); index++)
                    {
                        if (maskValues[index] > 0.0f)
                        {
                            uint32_t& srcPixel = m_cachedPixels[index];
                            AZ::Color pixel;
                            pixel.FromU32(srcPixel);

                            // Create our color variation.
                            AZ::Color destPixel = mapping.m_color1;
                            destPixel = destPixel.Lerp(mapping.m_color2, variationValues[index]);

                            // Alpha blend it back in with our built-up color so far.
                            pixel = pixel.Lerp(destPixel, maskValues[index]);

                            // Write it back into our cached buffer as a uint32
                            srcPixel = pixel.ToU32();
                        }
                    }
                }
            },
            true, jobContext);


        AZ::Job* finalJob = AZ::CreateJobFunction(
            [=]()
            {
                constexpr uint32_t BytesPerPixel = sizeof(uint32_t);

                AZ::RHI::ImageUpdateRequest imageUpdateRequest;
                imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = 0;
                imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = 0;
                imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = m_cachedPixelsWidth * BytesPerPixel;
                imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = m_cachedPixelsWidth * m_cachedPixelsHeight * BytesPerPixel;
                imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = m_cachedPixelsHeight;
                imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = m_cachedPixelsWidth;
                imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = m_cachedPixelsHeight;
                imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_depth = 1;
                imageUpdateRequest.m_sourceData = m_cachedPixels.data();
                imageUpdateRequest.m_image = m_generatedImage->GetRHIImage();

                m_generatedImage->UpdateImageContents(imageUpdateRequest);

                HandleMaterialStateChange();
            },
            true, jobContext);

        generatePixelDataJob->SetDependent(finalJob);
        generatePixelDataJob->Start();

        // TODO: The finalJob can't run on a separate thread right now, since Atom doesn't handle updating the image in the midst
        // of rendering.  The final update call to Atom needs to always be done from the main thread.
        // Right now, running on a separate thread will work some of the time, but not all of the time, due to timing.
        // finalJob->Start();
        finalJob->StartAndWaitForCompletion();
    }

    void TerrainGradientMacroMaterialComponent::OnCompositionChanged()
    {
        QueueRefresh();
    }

    void TerrainGradientMacroMaterialComponent::GetTerrainMacroMaterialColorData(
        uint32_t& width, uint32_t& height, AZStd::vector<AZ::Color>& pixels)
    {
        pixels.clear();
        width = 0;
        height = 0;

        if (m_cachedPixels.empty())
        {
            return;
        }

        width = m_cachedPixelsWidth;
        height = m_cachedPixelsHeight;

        pixels.reserve(width * height);

        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint32_t srcPixel = m_cachedPixels[(y * m_cachedPixelsWidth) + x];
                AZ::Color pixel(
                    static_cast<AZ::u8>(srcPixel & 0xFF), static_cast<AZ::u8>((srcPixel >> 8) & 0xFF),
                    static_cast<AZ::u8>((srcPixel >> 16) & 0xFF), static_cast<AZ::u8>((srcPixel >> 24) & 0xFF));
                pixels.push_back(pixel);
            }
        }
    }


} // namespace Terrain

#pragma optimize("", on)
