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
    enum class MaterialDefaultSlot
    {
        Default
    };

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

        void SetSlots(MaterialDefaultSlot);

        //! Sets an array of material slots to pick MaterialAssets for.
        //! Having multiple slots is required for assigning multiple materials
        //! on a mesh or heightfield object.
        //! @param slots List of labels for slots. It can be empty, in which case a slot will the default label "Entire Object" will be created.
        void SetSlots(const AZStd::vector<AZStd::string>& slots);

        void SetMaterialAsset(size_t slotIndex, const AZ::Data::Asset<MaterialAsset>& materialAsset);

        size_t GetSlotsCount() const;
        AZStd::string_view GetSlotName(size_t slotIndex) const;
        AZStd::vector<AZStd::string> GetSlotsNames() const;

        const AZ::Data::Asset<MaterialAsset> GetMaterialAsset(size_t slotIndex) const;

        //! Set if the material slots are editable in the edit context.
        void SetSlotsReadOnly(bool readOnly);

    protected:
        struct MaterialSlot
        {
            AZ_TYPE_INFO(Physics::MaterialSlots::MaterialSlot, "{B5AA3CC9-637F-44FB-A4AE-4621D37884BA}");

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZ::Data::Asset<MaterialAsset> m_materialAsset;

        private:
            friend class MaterialSlots;
            AZ::Data::AssetId GetDefaultMaterialAssetId() const;
            bool m_slotsReadOnly = false;
        };

        AZStd::vector<MaterialSlot> m_slots;
    };
} // namespace Physics
