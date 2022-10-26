/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Components/TerrainLayerSpawnerComponent.h>

namespace Terrain
{
    bool TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly() const
    {
        return !m_macroNormalAsset.GetId().IsValid();
    }

    void TerrainMacroMaterialConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context); serialize)
        {
            serialize->Class<TerrainMacroMaterialConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("MacroColor", &TerrainMacroMaterialConfig::m_macroColorAsset)
                ->Field("MacroNormal", &TerrainMacroMaterialConfig::m_macroNormalAsset)
                ->Field("NormalFlipX", &TerrainMacroMaterialConfig::m_normalFlipX)
                ->Field("NormalFlipY", &TerrainMacroMaterialConfig::m_normalFlipY)
                ->Field("NormalFactor", &TerrainMacroMaterialConfig::m_normalFactor)
                ->Field("Priority", &TerrainMacroMaterialConfig::m_priority)
                ;

            if (auto* editContext = serialize->GetEditContext(); editContext)
            {
                editContext
                    ->Class<TerrainMacroMaterialConfig>(
                        "Terrain Macro Material Component", "Provide a terrain macro material for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroColorAsset, "Color Texture",
                        "Terrain macro color texture for use by any terrain inside the bounding box on this entity.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroNormalAsset, "Normal Texture",
                        "Texture for defining surface normal direction. These will override normals generated from the geometry.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipX, "Normal Flip X",
                        "Flip tangent direction for this normal map.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipY, "Normal Flip Y",
                        "Flip bitangent direction for this normal map.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TerrainMacroMaterialConfig::m_normalFactor, "Normal Factor",
                        "Strength factor for scaling the normal map values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TerrainMacroMaterialConfig::m_priority, "Priority",
                        "Defines order macro materials are applied.  Larger numbers = higher priority")
                        ->Attribute(AZ::Edit::Attributes::Min, AreaConstants::s_priorityMin)
                        ->Attribute(AZ::Edit::Attributes::Max, AreaConstants::s_priorityMax)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, AreaConstants::s_priorityMin)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, AreaConstants::s_prioritySoftMax)
                    ;
            }
        }
    }

    void TerrainMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainMacroMaterialConfig::Reflect(context);
        TerrainMacroMaterialRequests::Reflect(context);
        
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainMacroMaterialComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainMacroMaterialComponent::m_configuration)
            ;
        }
    }

    TerrainMacroMaterialComponent::TerrainMacroMaterialComponent(const TerrainMacroMaterialConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainMacroMaterialComponent::Activate()
    {
        // Clear out our shape bounds and make sure the texture assets are queued to load.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();
        m_configuration.m_macroColorAsset.QueueLoad();
        m_configuration.m_macroNormalAsset.QueueLoad();

        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Listen for the texture assets to complete loading.
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroColorAsset.GetId());
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroNormalAsset.GetId());
    }

    void TerrainMacroMaterialComponent::Deactivate()
    {
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        m_configuration.m_macroColorAsset.Release();
        m_configuration.m_macroNormalAsset.Release();

        m_colorImage.reset();
        m_normalImage.reset();

        ClearImageModificationBuffer();

        // Send out any notifications as appropriate based on the macro material destruction.
        HandleMaterialStateChange();
    }

    bool TerrainMacroMaterialComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainMacroMaterialConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainMacroMaterialComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainMacroMaterialConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainMacroMaterialComponent::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
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
    }

    void TerrainMacroMaterialComponent::HandleMaterialStateChange()
    {
        // We only want our component to appear active during the time that the macro material is fully loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        // Color and normal data is considered ready if it's finished loading or if we don't have a texture specified
        bool colorReady = m_colorImage || (!m_configuration.m_macroColorAsset.GetId().IsValid());
        bool normalReady = m_normalImage || (!m_configuration.m_macroNormalAsset.GetId().IsValid());
        // If we don't have color or normal data, then we don't have *any* useful data, so don't activate the macro material.
        bool hasAnyData = m_configuration.m_macroColorAsset.GetId().IsValid() || m_configuration.m_macroNormalAsset.GetId().IsValid();

        bool wasPreviouslyActive = m_macroMaterialActive;
        bool isNowActive = colorReady && normalReady && hasAnyData;

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

            // Start listening for macro material modifications
            TerrainMacroMaterialModificationBus::Handler::BusConnect(GetEntityId());

            MacroMaterialData material = GetTerrainMacroMaterialData();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialCreated, GetEntityId(), material);
        }
        else if (wasPreviouslyActive && !isNowActive)
        {
            // Stop listening to macro material requests or shape changes, and send out a notification that we no longer have a valid
            // macro material.

            ClearImageModificationBuffer();

            TerrainMacroMaterialModificationBus::Handler::BusDisconnect();
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

    void TerrainMacroMaterialComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_configuration.m_macroColorAsset.GetId())
        {
            m_configuration.m_macroColorAsset = asset;
            m_colorImage = AZ::RPI::StreamingImage::FindOrCreate(m_configuration.m_macroColorAsset);
            m_colorImage->GetRHIImage()->SetName(AZ::Name(m_configuration.m_macroColorAsset.GetHint()));

            // Clear the texture asset reference to make sure we don't prevent hot-reloading.
            //m_configuration.m_macroColorAsset.Release();
        }
        else if (asset.GetId() == m_configuration.m_macroNormalAsset.GetId())
        {
            m_configuration.m_macroNormalAsset = asset;
            m_normalImage = AZ::RPI::StreamingImage::FindOrCreate(m_configuration.m_macroNormalAsset);
            m_normalImage->GetRHIImage()->SetName(AZ::Name(m_configuration.m_macroNormalAsset.GetHint()));

            // Clear the texture asset reference to make sure we don't prevent hot-reloading.
            //m_configuration.m_macroColorAsset.Release();
        }
        else
        {
        }

        HandleMaterialStateChange();
    }

    void TerrainMacroMaterialComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    MacroMaterialData TerrainMacroMaterialComponent::GetTerrainMacroMaterialData()
    {
        MacroMaterialData macroMaterial;

        macroMaterial.m_entityId = GetEntityId();
        macroMaterial.m_bounds = m_cachedShapeBounds;
        macroMaterial.m_colorImage = m_colorImage;
        macroMaterial.m_normalImage = m_normalImage;
        macroMaterial.m_normalFactor = m_configuration.m_normalFactor;
        macroMaterial.m_normalFlipX = m_configuration.m_normalFlipX;
        macroMaterial.m_normalFlipY = m_configuration.m_normalFlipY;
        macroMaterial.m_priority = m_configuration.m_priority;

        return macroMaterial;
    }

    void TerrainMacroMaterialComponent::GetTerrainMacroMaterialColorData(
        uint32_t& width, uint32_t& height, AZStd::vector<AZ::Color>& pixels)
    {
        pixels.clear();
        width = 0;
        height = 0;

        if (!m_colorImage || !m_colorImage->IsInitialized())
        {
            return;
        }

        auto assetId = m_colorImage->GetAssetId();
        auto assetType = m_colorImage->GetAssetType();
        auto macroImageAsset = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);
        macroImageAsset.BlockUntilLoadComplete();

        if (macroImageAsset.IsReady())
        {
            if (auto imageAsset = macroImageAsset.GetAs<AZ::RPI::StreamingImageAsset>(); imageAsset)
            {
                auto imageData = imageAsset->GetSubImageData(0, 0);

                if (!imageData.empty())
                {
                    AZ::RHI::Format format = imageAsset->GetImageDescriptor().m_format;
                    const auto numComponents = AZ::RHI::GetFormatComponentCount(format);

                    const AZ::RHI::ImageDescriptor& imageDescriptor = imageAsset->GetImageDescriptor();
                    width = imageDescriptor.m_size.m_width;
                    height = imageDescriptor.m_size.m_height;

                    pixels.reserve(width * height);

                    for (uint32_t y = 0; y < height; y++)
                    {
                        for (uint32_t x = 0; x < width; x++)
                        {
                            AZ::Color pixel = AZ::RPI::GetImageDataPixelValue<AZ::Color>(imageData, imageDescriptor, x, y);
                            pixels.push_back(pixel);
                        }
                    }
                }
            }
        }
    }

    uint32_t TerrainMacroMaterialComponent::GetMacroColorImageHeight() const
    {
        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();
        return imageDescriptor.m_size.m_height;
    }

    uint32_t TerrainMacroMaterialComponent::GetMacroColorImageWidth() const
    {
        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();
        return imageDescriptor.m_size.m_width;
    }

    AZ::Vector2 TerrainMacroMaterialComponent::GetMacroColorImagePixelsPerMeter() const
    {
        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();
        return AZ::Vector2(
            aznumeric_cast<float>(imageDescriptor.m_size.m_width) / m_cachedShapeBounds.GetXExtent(),
            aznumeric_cast<float>(imageDescriptor.m_size.m_height) / m_cachedShapeBounds.GetYExtent());
    }

    void TerrainMacroMaterialComponent::StartImageModification()
    {
        m_configuration.m_imageModificationActive = true;

        if (m_modifiedImageData.empty())
        {
            CreateImageModificationBuffer();
        }
    }

    void TerrainMacroMaterialComponent::EndImageModification()
    {
        m_configuration.m_imageModificationActive = false;
    }

    AZStd::vector<uint32_t>* TerrainMacroMaterialComponent::GetImageModificationBuffer()
    {
        // This will get replaced with safe/robust methods of modifying the image as paintbrush functionality
        // continues to get added to the Terrain Macro Material component.
        return &m_modifiedImageData;
    }

    void TerrainMacroMaterialComponent::CreateImageModificationBuffer()
    {
        if (!m_configuration.m_macroColorAsset->IsReady())
        {
            AZ_Error(
                "TerrainMacroMaterialComponent",
                false,
                "Color data is empty. Make sure the image asset is fully loaded before attempting to modify it.");
            return;
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_configuration.m_macroColorAsset->GetImageDescriptor();
        auto imageData = m_configuration.m_macroColorAsset->GetSubImageData(0, 0);

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        if (m_modifiedImageData.empty())
        {
            // Create a memory buffer for holding all of our modified image information.
            // We'll always use a buffer of floats to ensure that we're modifying at the highest precision possible.
            m_modifiedImageData.reserve(width * height);

            // Fill the buffer with all of our existing pixel values.
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    AZ::Color pixel = AZ::RPI::GetImageDataPixelValue<AZ::Color>(imageData, imageDescriptor, x, y);
                    m_modifiedImageData.emplace_back(pixel.ToU32());
                }
            }

            // Create an image descriptor describing our new buffer (correct width, height, and 8-bit RGB channels)
            auto modifiedImageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, width, height, AZ::RHI::Format::R8G8B8A8_UNORM);

            // Set our imageData pointer to point to our modified data buffer.
            auto modifiedImageData = AZStd::span<const uint8_t>(
                reinterpret_cast<uint8_t*>(m_modifiedImageData.data()), m_modifiedImageData.size() * sizeof(uint32_t));

            // Create the initial buffer for the downloaded color data
            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool =
                AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            const AZ::Name ModificationImageName = AZ::Name("ModifiedImage");
            m_colorImage =
                AZ::RPI::AttachmentImage::Create(*imagePool.get(), modifiedImageDescriptor, ModificationImageName, nullptr, nullptr);
            AZ_Error("Terrain", m_colorImage, "Failed to initialize the modification image buffer.");

            UpdateMacroMaterialTexture(0, 0, width - 1, height - 1);

            // Notify that the material has changed.
            MacroMaterialData material = GetTerrainMacroMaterialData();
            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, GetEntityId(), material);
        }
        else
        {
            // If this triggers, we've somehow gotten our image modification buffer out of sync with the image descriptor information.
            AZ_Assert(m_modifiedImageData.size() == (width * height), "Image modification buffer exists but is the wrong size.");
        }
    }

    void TerrainMacroMaterialComponent::ClearImageModificationBuffer()
    {
        m_modifiedImageData.resize(0);
    }

    bool TerrainMacroMaterialComponent::ModificationBufferIsActive() const
    {
        return (!m_modifiedImageData.empty());
    }

    void TerrainMacroMaterialComponent::SetPixelValueByPosition(const AZ::Vector3& position, AZ::Color value)
    {
        SetPixelValuesByPosition(AZStd::span<const AZ::Vector3>(&position, 1), AZStd::span<AZ::Color>(&value, 1));
    }

    void TerrainMacroMaterialComponent::UpdateMacroMaterialTexture(
        uint32_t leftPixel, uint32_t topPixel, uint32_t rightPixel, uint32_t bottomPixel)
    {
        // 0-pixel row or 0-pixel column means nothing to update.
        if ((bottomPixel <= topPixel) || (rightPixel <= leftPixel))
        {
            return;
        }


        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& imageWidth = imageDescriptor.m_size.m_width;
        const auto& imageHeight = imageDescriptor.m_size.m_height;

        // By default, we'll update the entire image from our modification buffer.
        uint32_t updateLeftPixel = 0;
        uint32_t updateTopPixel = 0;
        uint32_t updateWidth = imageWidth;
        uint32_t updateHeight = imageHeight;
        void* sourceData = m_modifiedImageData.data();

        uint32_t modifiedWidth = rightPixel - leftPixel + 1;
        uint32_t modifiedHeight = bottomPixel - topPixel + 1;

        AZStd::vector<uint32_t> tempBuffer;

        // If less than 50% of the total pixels have changed, we'll create a temporary buffer and only upload the portion
        // of the image that changed. At some point the overhead of creating the temporary buffer outweighs the cost of just
        // reuploading the entire image.
        constexpr float TotalPixelsChangedPercent = 0.50f;
        if (((modifiedWidth * modifiedHeight) / (imageWidth * imageHeight)) <= TotalPixelsChangedPercent)
        {
            updateLeftPixel = leftPixel;
            updateTopPixel = topPixel;
            updateWidth = modifiedWidth;
            updateHeight = modifiedHeight;

            tempBuffer.resize_no_construct(updateWidth * updateHeight);
            for (uint32_t y = 0; y < updateHeight; y++)
            {
                memcpy(
                    &tempBuffer[(y * updateWidth)],
                    &m_modifiedImageData[((y + topPixel) * imageWidth) + leftPixel],
                    updateWidth * sizeof(uint32_t));
            }

            sourceData = tempBuffer.data();
        }

        const uint32_t BytesPerPixel = 4;
        AZ::RHI::ImageUpdateRequest imageUpdateRequest;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = updateLeftPixel;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = updateTopPixel;
        imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = updateWidth * BytesPerPixel;
        imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = updateWidth * updateHeight * BytesPerPixel;
        imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = updateHeight;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = updateWidth;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = updateHeight;
        imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_depth = 1;
        imageUpdateRequest.m_sourceData = sourceData;
        imageUpdateRequest.m_image = m_colorImage->GetRHIImage();
        m_colorImage->UpdateImageContents(imageUpdateRequest);
    }

    void TerrainMacroMaterialComponent::SetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<const AZ::Color> values)
    {
        if (m_modifiedImageData.empty())
        {
            AZ_Error("ImageGradientComponent", false, "Image modification mode needs to be started before the image values can be set.");
            return;
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        // No pixels, so nothing to modify.
        if ((width == 0) || (height == 0))
        {
            return;
        }

        uint32_t topPixel = height;
        uint32_t bottomPixel = 0;
        uint32_t leftPixel = width;
        uint32_t rightPixel = 0;

        for (size_t index = 0; index < positions.size(); index++)
        {
            auto pixelX = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(width - 1),
                (positions[index].GetX() - m_cachedShapeBounds.GetMin().GetX()) / m_cachedShapeBounds.GetXExtent());

            auto pixelY = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(height - 1),
                (positions[index].GetY() - m_cachedShapeBounds.GetMin().GetY()) / m_cachedShapeBounds.GetYExtent());

            auto x = aznumeric_cast<AZ::u32>(pixelX) % width;
            auto y = aznumeric_cast<AZ::u32>(pixelY) % height;

            // Flip the y because images are stored in reverse of our world axes
            y = (height - 1) - y;

            topPixel = AZStd::min(topPixel, y);
            bottomPixel = AZStd::max(bottomPixel, y);
            leftPixel = AZStd::min(leftPixel, x);
            rightPixel = AZStd::max(rightPixel, x);

            // Modify the correct pixel in our modification buffer.
            m_modifiedImageData[(y * width) + x] = values[index].ToU32();
        }

        UpdateMacroMaterialTexture(leftPixel, topPixel, rightPixel, bottomPixel);
    }

    void TerrainMacroMaterialComponent::GetPixelIndicesForPositions(
        AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const
    {
        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            auto pixelX = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(width - 1),
                (positions[index].GetX() - m_cachedShapeBounds.GetMin().GetX()) / m_cachedShapeBounds.GetXExtent());

            auto pixelY = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(height - 1),
                (positions[index].GetY() - m_cachedShapeBounds.GetMin().GetY()) / m_cachedShapeBounds.GetYExtent());

            auto x = aznumeric_cast<AZ::u32>(pixelX) % width;
            auto y = aznumeric_cast<AZ::u32>(pixelY) % height;

            // Flip the y because images are stored in reverse of our world axes
            y = (height - 1) - y;

            outIndices[index] = PixelIndex(aznumeric_cast<int16_t>(x), aznumeric_cast<int16_t>(y));
        }
    }

    void TerrainMacroMaterialComponent::GetPixelValuesByPixelIndex(
        AZStd::span<const PixelIndex> positions, AZStd::span<AZ::Color> outValues) const
    {
        AZ_Assert(!m_modifiedImageData.empty(), "Pixel values are only available during modifications.");

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                uint8_t r = (m_modifiedImageData[(y * width) + x] >> 0) & 0xFF;
                uint8_t g = (m_modifiedImageData[(y * width) + x] >> 8) & 0xFF;
                uint8_t b = (m_modifiedImageData[(y * width) + x] >> 16) & 0xFF;
                uint8_t a = (m_modifiedImageData[(y * width) + x] >> 24) & 0xFF;
                outValues[index] = AZ::Color(r, g, b, a);
            }
        }
    }

    void TerrainMacroMaterialComponent::SetPixelValueByPixelIndex(const PixelIndex& position, AZ::Color value)
    {
        SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex>(&position, 1), AZStd::span<AZ::Color>(&value, 1));
    }

    void TerrainMacroMaterialComponent::SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<const AZ::Color> values)
    {
        if (m_modifiedImageData.empty())
        {
            AZ_Error("ImageGradientComponent", false, "Image modification mode needs to be started before the image values can be set.");
            return;
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        // No pixels, so nothing to modify.
        if ((width == 0) || (height == 0))
        {
            return;
        }

        uint32_t topPixel = height;
        uint32_t bottomPixel = 0;
        uint32_t leftPixel = width;
        uint32_t rightPixel = 0;

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                // Modify the correct pixel in our modification buffer.
                m_modifiedImageData[(y * width) + x] = values[index].ToU32();

                topPixel = AZStd::min(topPixel, aznumeric_cast<uint32_t>(y));
                bottomPixel = AZStd::max(bottomPixel, aznumeric_cast<uint32_t>(y));
                leftPixel = AZStd::min(leftPixel, aznumeric_cast<uint32_t>(x));
                rightPixel = AZStd::max(rightPixel, aznumeric_cast<uint32_t>(x));
            }
        }

        UpdateMacroMaterialTexture(leftPixel, topPixel, rightPixel, bottomPixel);
    }


}
