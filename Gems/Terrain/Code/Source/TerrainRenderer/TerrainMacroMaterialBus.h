/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace Terrain
{
    struct MacroMaterialData final
    {
        AZ_TYPE_INFO(MacroMaterialData, "{DC68E20A-3251-4E4E-8BC7-F6A2521FEF46}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_entityId;
        AZ::Aabb m_bounds = AZ::Aabb::CreateNull();

        AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
        AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
        bool m_normalFlipX{ false };
        bool m_normalFlipY{ false };
        float m_normalFactor{ 0.0f };
        int32_t m_priority{ 0 };
    };

    /**
    * Request terrain macro material data.
    */
    class TerrainMacroMaterialRequests
        : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);
    
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~TerrainMacroMaterialRequests() = default;

        // Get the terrain macro material and the region that it covers.
        virtual MacroMaterialData GetTerrainMacroMaterialData() = 0;

        virtual void GetTerrainMacroMaterialColorData(uint32_t& width, uint32_t& height, AZStd::vector<AZ::Color>& pixels) = 0;

        virtual uint32_t GetMacroColorImageHeight() const
        {
            return 0;
        }

        virtual uint32_t GetMacroColorImageWidth() const
        {
            return 0;
        }

        virtual AZ::Vector2 GetMacroColorImagePixelsPerMeter() const
        {
            return AZ::Vector2(0.0f, 0.0f);
        }
    };

    using TerrainMacroMaterialRequestBus = AZ::EBus<TerrainMacroMaterialRequests>;
    
    /**
    * Notifications for when the terrain macro material data changes.
    */
    class TerrainMacroMaterialNotifications : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnTerrainMacroMaterialCreated(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        virtual void OnTerrainMacroMaterialChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity, [[maybe_unused]] const MacroMaterialData& macroMaterial)
        {
        }

        virtual void OnTerrainMacroMaterialRegionChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion)
        {
        }

        virtual void OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId macroMaterialEntity)
        {
        }
    };
    using TerrainMacroMaterialNotificationBus = AZ::EBus<TerrainMacroMaterialNotifications>;

    using PixelIndex = AZStd::pair<int16_t, int16_t>;

    //! EBus that can be used to modify the image data for a Terrain Macro Material texture.
    class TerrainMacroMaterialModifications : public AZ::ComponentBus
    {
    public:
        // Overrides the default AZ::EBusTraits handler policy to allow only one listener per entity.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Start an image modification session.
        //! This will create a modification buffer that contains an uncompressed copy of the current image data.
        virtual void StartImageModification() = 0;

        //! Finish an image modification session.
        //! Currently does nothing, but might eventually need to perform some cleanup logic.
        virtual void EndImageModification() = 0;

        //! Given a list of world positions, return a list of pixel indices into the image.
        //! @param positions The list of world positions to query
        //! @param outIndices [out] The list of output PixelIndex values giving the (x,y) pixel coordinates for each world position.
        virtual void GetPixelIndicesForPositions(
            AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const = 0;

        //! Get the image pixel values at a list of pixel indices.
        //! Unlike GetValues on the GradientRequestBus, this will always use point sampling regardless of
        //! the Image Gradient sampler type because the specific pixel values are being requested.
        //! @param positions The list of pixel indices to query
        //! @param outValues [out] The list of output values. This list is expected to be the same size as the positions list.
        virtual void GetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<AZ::Color> outValues) const = 0;

        //! Set the value at the given world position.
        //! @param position The world position to set the value at.
        //! @param value The value to set it to.
        virtual void SetPixelValueByPosition(const AZ::Vector3& position, AZ::Color value) = 0;

        //! Set the value at the given pixel index.
        //! @param index The pixel index to set the value at.
        //! @param value The value to set it to.
        virtual void SetPixelValueByPixelIndex(const PixelIndex& index, AZ::Color value) = 0;

        //! Given a list of world positions, set the pixels at those positions to the given values.
        //! @param positions The list of world positions to set the values for.
        //! @param values The list of values to set. This list is expected to be the same size as the positions list.
        virtual void SetPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<const AZ::Color> values) = 0;

        //! Given a list of pixel indices, set those pixels to the given values.
        //! @param indicdes The list of pixel indices to set the values for.
        //! @param values The list of values to set. This list is expected to be the same size as the positions list.
        virtual void SetPixelValuesByPixelIndex(AZStd::span<const PixelIndex> indices, AZStd::span<const AZ::Color> values) = 0;
    };

    using TerrainMacroMaterialModificationBus = AZ::EBus<TerrainMacroMaterialModifications>;
}
