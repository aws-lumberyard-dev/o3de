/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    //! The class is used to store a list of material assets.
    //! Each material will be assigned to a slot and when reflected
    //! to edit context it will show it for each slot entry.
    class MaterialSlots
    {
    public:
        AZ_CLASS_ALLOCATOR(Physics::MaterialSlots, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::MaterialSlots, "{8A0D64CB-C98E-42E3-96A9-B81D7118CA6F}");

        MaterialSlots();
        virtual ~MaterialSlots() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! Sets an array of material slots to pick MaterialAssets for.
        //! Having multiple slots is required for assigning multiple materials
        //! on a mesh or heightfield object.
        //! @param slots List of labels for slots. It can be empty, in which case a slot will the default label "Entire Object" will be created.
        void SetSlots(const AZStd::vector<AZStd::string>& slots);

        void SetMaterialAsset(size_t slotIndex, const AZ::Data::Asset<MaterialAsset>& materialAsset);

        size_t GetSlotsCount() const;

        AZStd::string_view GetSlotName(size_t slotIndex) const;

        const AZ::Data::Asset<MaterialAsset> GetMaterialAsset(size_t slotIndex) const;

        //! Set if the material slots are editable in the edit context.
        void SetSlotsReadOnly(bool readOnly);

    protected:
        AZStd::vector<AZStd::string> m_slots; //!< List of labels for slots.
        AZStd::vector<AZ::Data::Asset<MaterialAsset>> m_materialAssets; //!< List of material assets assigned to slots.

    private:
        AZStd::string GetSlotLabel(int index) const;
        AZ::Data::AssetId GetDefaultMaterialAssetId() const;

        bool m_slotsReadOnly = false;
    };
} // namespace Physics
