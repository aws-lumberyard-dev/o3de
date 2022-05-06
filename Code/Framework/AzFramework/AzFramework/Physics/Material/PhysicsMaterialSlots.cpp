/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetSerializer.h>

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>

namespace Physics
{
    namespace
    {
        const char* const EntireObjectSlotName = "Entire object";
    }

    void MaterialSlots::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialSlots>()
                ->Version(1)
                ->Field("Slots", &MaterialSlots::m_slots)
                ->Field("MaterialAssets", &MaterialSlots::m_materialAssets)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialSlots>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSlots::m_materialAssets, "Physics Materials",
                        "Select which physics materials to use for each slot.")
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &MaterialSlots::GetSlotLabel)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ_CRC_CE("ValueText"), " ")
                        ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, &MaterialSlots::m_slotsReadOnly)
                        ->ElementAttribute(AZ::Edit::Attributes::DefaultAsset, &MaterialSlots::GetDefaultMaterialAssetId)
                        ->ElementAttribute(AZ_CRC_CE("EditButton"), "")
                        ->ElementAttribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                        ->ElementAttribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ;
            }
        }
    }

    MaterialSlots::MaterialSlots()
    {
        SetSlots({}); // Create default slot
    }

    void MaterialSlots::SetSlots(const AZStd::vector<AZStd::string>& slots)
    {
        if (slots.empty())
        {
            m_slots = { EntireObjectSlotName };
        }
        else
        {
            m_slots = slots;
        }

        m_materialAssets.resize(m_slots.size());
    }

    void MaterialSlots::SetMaterialAsset(size_t slotIndex, const AZ::Data::Asset<MaterialAsset>& materialAsset)
    {
        if (slotIndex < m_materialAssets.size())
        {
            m_materialAssets[slotIndex] = materialAsset;
        }
    }

    size_t MaterialSlots::GetSlotsCount() const
    {
        return m_slots.size();
    }

    AZStd::string_view MaterialSlots::GetSlotName(size_t slotIndex) const
    {
        if (slotIndex < m_slots.size())
        {
            return m_slots[slotIndex];
        }
        else
        {
            return "<error>";
        }
    }

    const AZ::Data::Asset<MaterialAsset> MaterialSlots::GetMaterialAsset(size_t slotIndex) const
    {
        if (slotIndex < m_materialAssets.size())
        {
            return m_materialAssets[slotIndex];
        }
        else
        {
            return {};
        }
    }

    void MaterialSlots::SetSlotsReadOnly(bool readOnly)
    {
        m_slotsReadOnly = readOnly;
    }

    AZStd::string MaterialSlots::GetSlotLabel(int index) const
    {
        return GetSlotName(static_cast<size_t>(index));
    }

    AZ::Data::AssetId MaterialSlots::GetDefaultMaterialAssetId() const
    {
        // Used for Edit Context.
        // When the physics material asset property doesn't have an asset assigned it
        // will show "(default)" to indicate that the default material will be used.
        return {};
    }
} // namespace Physics
