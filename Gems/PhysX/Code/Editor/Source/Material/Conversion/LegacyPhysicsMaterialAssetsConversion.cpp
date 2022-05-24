/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Spawnable/Spawnable.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Source/EditorColliderComponent.h>
#include <Source/EditorShapeColliderComponent.h>
#include <Source/EditorHeightfieldColliderComponent.h>
#include <Source/PhysXCharacters/Components/EditorCharacterControllerComponent.h>

namespace PhysX
{
    void FixAssetsUsingPhysicsLegacyMaterials(const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC(
        "ed_physxFixAssetsUsingPhysicsLegacyMaterials",
        FixAssetsUsingPhysicsLegacyMaterials,
        AZ::ConsoleFunctorFlags::Null,
        "Finds assets that reference legacy physics material ids and fixes them by using new physics material assets.");

    static const AZ::TypeId EditorBlastFamilyComponentTypeId = AZ::TypeId::CreateString("{ECB1689A-2B65-44D1-9227-9E62962A7FF7}");
    static const AZ::TypeId EditorTerrainPhysicsColliderComponentTypeId = AZ::TypeId::CreateString("{C43FAB8F-3968-46A6-920E-E84AEDED3DF5}");

    AZStd::optional<AZStd::string> GetFullSourceAssetPathById(AZ::Data::AssetId assetId);

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabEntities(AzToolsFramework::Prefab::PrefabDom& prefabDom)
    {
        if (!prefabDom.IsObject())
        {
            return {};
        }

        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> entities;
        entities.reserve(prefabDom.MemberCount());

        if (auto entitiesIter = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
            entitiesIter != prefabDom.MemberEnd() && entitiesIter->value.IsObject())
        {
            for (auto entityIter = entitiesIter->value.MemberBegin(); entityIter != entitiesIter->value.MemberEnd(); ++entityIter)
            {
                if (entityIter->value.IsObject())
                {
                    entities.push_back(&entityIter->value);
                }
            }
        }

        return entities;
    }

    AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> GetPrefabComponents(
        const AZ::TypeId& componentTypeId, AzToolsFramework::Prefab::PrefabDomValue& prefabEntity)
    {
        AZStd::vector<AzToolsFramework::Prefab::PrefabDomValue*> components;
        components.reserve(prefabEntity.MemberCount());

        if (auto componentsIter = prefabEntity.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName);
            componentsIter != prefabEntity.MemberEnd() && componentsIter->value.IsObject())
        {
            for (auto componentIter = componentsIter->value.MemberBegin(); componentIter != componentsIter->value.MemberEnd();
                 ++componentIter)
            {
                if (!componentIter->value.IsObject())
                {
                    continue;
                }

                // Check the component type
                const auto typeFieldIter = componentIter->value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::TypeName);
                if (typeFieldIter == componentIter->value.MemberEnd())
                {
                    continue;
                }

                AZ::Uuid typeId = AZ::Uuid::CreateNull();
                AZ::JsonSerialization::LoadTypeId(typeId, typeFieldIter->value);

                // Filter by component type
                if (typeId == componentTypeId)
                {
                    components.push_back(&componentIter->value);
                }
            }
        }

        return components;
    }

    AzToolsFramework::Prefab::PrefabDomValue* FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return nullptr;
        }

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return nullptr;
        }

        for (size_t i = 1; i < memberChain.size(); ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return nullptr;
            }
            memberIter = memberFoundIter;
        }
        return &memberIter->value;
    }

    const AzToolsFramework::Prefab::PrefabDomValue* FindConstMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        return FindMemberChainInPrefabComponent(memberChain, const_cast<AzToolsFramework::Prefab::PrefabDomValue&>(prefabComponent));
    }

    void RemoveMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        if (memberChain.empty())
        {
            return;
        }
        else if (memberChain.size() == 1)
        {
            prefabComponent.RemoveMember(memberChain[0].c_str());
            return;
        }

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return;
        }

        for (size_t i = 1; i < memberChain.size()-1; ++i)
        {
            auto memberFoundIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberFoundIter == memberIter->value.MemberEnd())
            {
                return;
            }
            memberIter = memberFoundIter;
        }

        memberIter->value.RemoveMember(memberChain.back().c_str());
    }

    template<class T>
    bool LoadObjectFromPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent, T& object)
    {
        const auto* member = FindConstMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        auto result = AZ::JsonSerialization::Load(&object, azrtti_typeid<T>(), *member);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    template<class T>
    bool StoreObjectToPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabComponent,
        const T& object)
    {
        auto* member = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (!member)
        {
            return false;
        }

        T defaultObject;

        auto result =
            AZ::JsonSerialization::Store(*member, prefabDom.GetAllocator(), &object, &defaultObject, azrtti_typeid<T>());

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    bool PrefabHasLegacyMaterialId(AzToolsFramework::Prefab::PrefabDom& prefabDom)
    {
        auto componentHasLegacyMaterialSelection = [](const AzToolsFramework::Prefab::PrefabDomValue& component, const AZStd::vector<AZStd::string>& memberChain)
        {
            PhysicsLegacy::MaterialSelection legacyMaterialSelection;
            if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(memberChain, component, legacyMaterialSelection))
            {
                return AZStd::any_of(
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.begin(),
                    legacyMaterialSelection.m_materialIdsAssignedToSlots.end(),
                    [](const PhysicsLegacy::MaterialId& legacyMaterialId)
                    {
                        return !legacyMaterialId.m_id.IsNull();
                    });
            }
            return false;
        };

        auto componentHasLegacyMaterialId = [](const AzToolsFramework::Prefab::PrefabDomValue& component, const AZStd::vector<AZStd::string>& memberChain)
        {
            PhysicsLegacy::MaterialId legacyMaterialId;
            if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(memberChain, component, legacyMaterialId))
            {
                return !legacyMaterialId.m_id.IsNull();
            }
            return false;
        };

        for (auto* entity : GetPrefabEntities(prefabDom))
        {
            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorColliderComponent>(), *entity))
            {
                if (componentHasLegacyMaterialSelection(*component, { "ColliderConfiguration", "MaterialSelection" }))
                {
                    return true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorShapeColliderComponent>(), *entity))
            {
                if (componentHasLegacyMaterialSelection(*component, { "ColliderConfiguration", "MaterialSelection" }))
                {
                    return true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorHeightfieldColliderComponent>(), *entity))
            {
                if (componentHasLegacyMaterialSelection(*component, { "ColliderConfiguration", "MaterialSelection" }))
                {
                    return true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorCharacterControllerComponent>(), *entity))
            {
                if (componentHasLegacyMaterialSelection(*component, { "Configuration", "Material" }))
                {
                    return true;
                }
            }

            for (auto* component : GetPrefabComponents(EditorTerrainPhysicsColliderComponentTypeId, *entity))
            {
                if (componentHasLegacyMaterialSelection(*component, { "Configuration", "DefaultMaterial" }))
                {
                    return true;
                }

                const auto* mappingMember = FindConstMemberChainInPrefabComponent({ "Configuration", "Mappings" }, *component);
                if (mappingMember)
                {
                    for (rapidjson_ly::SizeType i = 0; i < mappingMember->Size(); ++i)
                    {
                        if (componentHasLegacyMaterialId((*mappingMember)[i], { "Material" }))
                        {
                            return true;
                        }
                    }
                }
            }

            for (auto* component : GetPrefabComponents(EditorBlastFamilyComponentTypeId, *entity))
            {
                if (componentHasLegacyMaterialId(*component, { "PhysicsMaterial" }))
                {
                    return true;
                }
            }
        }

        return false;
    }

    struct PrefabInfo
    {
        AzToolsFramework::Prefab::TemplateId m_templateId;
        AzToolsFramework::Prefab::Template* m_template = nullptr;
        AZStd::string m_prefabFullPath;
    };

    AZStd::vector<PrefabInfo> CollectPrefabsWithLegacyMaterials()
    {
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials;

        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&prefabsWithLegacyMaterials, prefabLoader,
             prefabSystemComponent](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != AzFramework::Spawnable::RTTI_Type())
            {
                return;
            }

            AZStd::optional<AZStd::string> assetFullPath = GetFullSourceAssetPathById(assetId);
            if (!assetFullPath.has_value())
            {
                return;
            }

            if (auto templateId = prefabLoader->LoadTemplateFromFile(assetFullPath->c_str());
                templateId != AzToolsFramework::Prefab::InvalidTemplateId)
            {
                if (auto templateResult = prefabSystemComponent->FindTemplate(templateId); templateResult.has_value())
                {
                    if (AzToolsFramework::Prefab::Template& templateRef = templateResult->get();
                        PrefabHasLegacyMaterialId(templateRef.GetPrefabDom()))
                    {
                        prefabsWithLegacyMaterials.push_back({ templateId, &templateRef, AZStd::move(*assetFullPath) });
                    }
                }
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return prefabsWithLegacyMaterials;
    }

    using LegacyMaterialIdToNewAssetIdMap = AZStd::unordered_map<AZ::Uuid, AZ::Data::AssetId>;

    LegacyMaterialIdToNewAssetIdMap CollectConvertedMaterialIds()
    {
        LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB assetEnumerationCB =
            [&legacyMaterialIdToNewAssetIdMap](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            if (assetInfo.m_assetType != Physics::MaterialAsset::RTTI_Type())
            {
                return;
            }

            AZ::Data::Asset<Physics::MaterialAsset> materialAsset(assetId, assetInfo.m_assetType);
            materialAsset.QueueLoad();
            materialAsset.BlockUntilLoadComplete();

            if (materialAsset.IsReady())
            {
                if (const AZ::Uuid& legacyPhysicsMaterialId = materialAsset->GetLegacyPhysicsMaterialId().m_id;
                    !legacyPhysicsMaterialId.IsNull())
                {
                    legacyMaterialIdToNewAssetIdMap.emplace(legacyPhysicsMaterialId, assetId);
                }
            }
            else
            {
                AZ_Warning("PhysXMaterialConversion", false, "Unable to load physics material asset '%s'.", assetInfo.m_relativePath.c_str());
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return legacyMaterialIdToNewAssetIdMap;
    }

    AZ::Data::Asset<Physics::MaterialAsset> ConvertLegacyMaterialIdToMaterialAsset(
        const PhysicsLegacy::MaterialId& legacyMaterialId,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        if (legacyMaterialId.m_id.IsNull())
        {
            return {};
        }

        auto it = legacyMaterialIdToNewAssetIdMap.find(legacyMaterialId.m_id);
        if (it == legacyMaterialIdToNewAssetIdMap.end())
        {
            AZ_Warning("PhysXMaterialConversion", false, "Unable to find a physics material asset to replace legacy material id '%s' with.",
                legacyMaterialId.m_id.ToString<AZStd::string>().c_str());
            return {};
        }

        const AZ::Data::AssetId newMaterialAssetId = it->second;

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newMaterialAssetId);
        AZ::Data::Asset<Physics::MaterialAsset> newMaterialAsset(newMaterialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);

        return newMaterialAsset;
    }

    Physics::MaterialSlots ConvertLegacyMaterialSelectionToMaterialSlots(
        const PhysicsLegacy::MaterialSelection& legacyMaterialSelection,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        if (legacyMaterialSelection.m_materialIdsAssignedToSlots.empty())
        {
            return {};
        }

        Physics::MaterialSlots newMaterialSlots;

        if (legacyMaterialSelection.m_materialIdsAssignedToSlots.size() == 1)
        {
            // MaterialSlots by default has one slot called "Entire Object", keep that
            // name if the selection only has one entry.
            newMaterialSlots.SetMaterialAsset(0,
                ConvertLegacyMaterialIdToMaterialAsset(
                    legacyMaterialSelection.m_materialIdsAssignedToSlots[0], legacyMaterialIdToNewAssetIdMap));
        }
        else
        {
            AZStd::vector<AZStd::string> slotNames;
            slotNames.reserve(legacyMaterialSelection.m_materialIdsAssignedToSlots.size());
            for (size_t i = 0; i < legacyMaterialSelection.m_materialIdsAssignedToSlots.size(); ++i)
            {
                // Using Material 1, Material 2, etc. for slot names when there is more than one entry.
                slotNames.push_back(AZStd::string::format("Material %d", i+1));
            }

            newMaterialSlots.SetSlots(slotNames);
            for (size_t i = 0; i < legacyMaterialSelection.m_materialIdsAssignedToSlots.size(); ++i)
            {
                newMaterialSlots.SetMaterialAsset(i,
                    ConvertLegacyMaterialIdToMaterialAsset(
                        legacyMaterialSelection.m_materialIdsAssignedToSlots[i], legacyMaterialIdToNewAssetIdMap));
            }
        }

        return newMaterialSlots;
    }

    void FixPrefabPhysicsMaterials(
        PrefabInfo& prefabWithLegacyMaterials,
        const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZ_TracePrintf("PhysXMaterialConversion", "Fixing prefab '%s'.\n", prefabWithLegacyMaterials.m_prefabFullPath.c_str());

        auto fixComponentPhysicsMaterialSelection =
            [&prefabWithLegacyMaterials, &legacyMaterialIdToNewAssetIdMap](
                AzToolsFramework::Prefab::PrefabDomValue& component,
                const AZStd::vector<AZStd::string>& oldMemberChain,
                const AZStd::vector<AZStd::string>& newMemberChain)
        {
            PhysicsLegacy::MaterialSelection legacyMaterialSelection;
            if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(oldMemberChain, component, legacyMaterialSelection))
            {
                const Physics::MaterialSlots materialSlots = ConvertLegacyMaterialSelectionToMaterialSlots(legacyMaterialSelection, legacyMaterialIdToNewAssetIdMap);

                if (!StoreObjectToPrefabComponent<Physics::MaterialSlots>(
                    newMemberChain, prefabWithLegacyMaterials.m_template->GetPrefabDom(), component, materialSlots))
                {
                    AZ_Warning("PhysXMaterialConversion", false, "Unable to set physics material slots to prefab.");
                    return false;
                }

                // Remove legacy material selection field
                RemoveMemberChainInPrefabComponent(oldMemberChain, component);

                AZ_TracePrintf("PhysXMaterialConversion", "Legacy material selection will be replaced by physics material slots.\n");

                return true;
            }
            return false;
        };

        auto fixComponentPhysicsMaterialId =
            [&prefabWithLegacyMaterials, &legacyMaterialIdToNewAssetIdMap](
                AzToolsFramework::Prefab::PrefabDomValue& component,
                const AZStd::vector<AZStd::string>& oldMemberChain,
                const AZStd::vector<AZStd::string>& newMemberChain)
        {
            PhysicsLegacy::MaterialId legacyMaterialId;
            if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(oldMemberChain, component, legacyMaterialId))
            {
                AZ::Data::Asset<Physics::MaterialAsset> materialAsset = ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

                if (!StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                    newMemberChain, prefabWithLegacyMaterials.m_template->GetPrefabDom(), component, materialAsset))
                {
                    AZ_Warning("PhysXMaterialConversion", false, "Unable to set physics material asset to prefab.")
                    return false;
                }

                // Remove legacy material id field
                RemoveMemberChainInPrefabComponent(oldMemberChain, component);

                AZ_TracePrintf("PhysXMaterialConversion", "Legacy material id '%s' will be replaced by physics material asset '%s'.\n",
                    legacyMaterialId.m_id.ToString<AZStd::string>().c_str(),
                    materialAsset.GetHint().c_str());

                return true;
            }
            return false;
        };

        bool prefabDomModified = false;
        for (auto* entity : GetPrefabEntities(prefabWithLegacyMaterials.m_template->GetPrefabDom()))
        {
            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorColliderComponent>(), *entity))
            {
                if (fixComponentPhysicsMaterialSelection(*component,
                    { "ColliderConfiguration", "MaterialSelection" }, { "ColliderConfiguration", "MaterialSlots" }))
                {
                    prefabDomModified = true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorShapeColliderComponent>(), *entity))
            {
                if (fixComponentPhysicsMaterialSelection(*component,
                    { "ColliderConfiguration", "MaterialSelection" }, { "ColliderConfiguration", "MaterialSlots" }))
                {
                    prefabDomModified = true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorHeightfieldColliderComponent>(), *entity))
            {
                if (fixComponentPhysicsMaterialSelection(*component,
                    { "ColliderConfiguration", "MaterialSelection" }, { "ColliderConfiguration", "MaterialSlots" }))
                {
                    prefabDomModified = true;
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorCharacterControllerComponent>(), *entity))
            {
                if (fixComponentPhysicsMaterialSelection(*component,
                    { "Configuration", "Material" }, { "Configuration", "MaterialSlots" }))
                {
                    prefabDomModified = true;
                }
            }

            for (auto* component : GetPrefabComponents(EditorTerrainPhysicsColliderComponentTypeId, *entity))
            {
                PhysicsLegacy::MaterialSelection legacyDefaultMaterialSelection;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>({ "Configuration", "DefaultMaterial" }, *component, legacyDefaultMaterialSelection))
                {
                    if (!legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots.empty())
                    {
                        PhysicsLegacy::MaterialId legacyMaterialId = legacyDefaultMaterialSelection.m_materialIdsAssignedToSlots[0];

                        AZ::Data::Asset<Physics::MaterialAsset> materialAsset = ConvertLegacyMaterialIdToMaterialAsset(legacyMaterialId, legacyMaterialIdToNewAssetIdMap);

                        if (StoreObjectToPrefabComponent<AZ::Data::Asset<Physics::MaterialAsset>>(
                            { "Configuration", "DefaultMaterialAsset" }, prefabWithLegacyMaterials.m_template->GetPrefabDom(), *component, materialAsset))
                        {
                            // Remove legacy material selection field
                            RemoveMemberChainInPrefabComponent({ "Configuration", "DefaultMaterial" }, *component);

                            AZ_TracePrintf("PhysXMaterialConversion", "Legacy selection with one material (id '%s') will be replaced by physics material asset '%s'.\n",
                                legacyMaterialId.m_id.ToString<AZStd::string>().c_str(),
                                materialAsset.GetHint().c_str());

                            prefabDomModified = true;
                        }
                        else
                        {
                            AZ_Warning("PhysXMaterialConversion", false, "Unable to set physics material asset to prefab.");
                        }
                    }
                }

                auto* mappingMember = FindMemberChainInPrefabComponent({ "Configuration", "Mappings" }, *component);
                if (mappingMember)
                {
                    for (rapidjson_ly::SizeType i = 0; i < mappingMember->Size(); ++i)
                    {
                        if (fixComponentPhysicsMaterialId((*mappingMember)[i],
                            { "Material" }, { "MaterialAsset" }))
                        {
                            prefabDomModified = true;
                        }
                    }
                }
            }

            for (auto* component : GetPrefabComponents(EditorBlastFamilyComponentTypeId, *entity))
            {
                if (fixComponentPhysicsMaterialId(*component,
                    { "PhysicsMaterial" }, { "PhysicsMaterialAsset" }))
                {
                    prefabDomModified = true;
                }
            }
        }

        if (prefabDomModified)
        {
            auto* prefabSystemComponent = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

            prefabWithLegacyMaterials.m_template->MarkAsDirty(true);
            prefabSystemComponent->PropagateTemplateChanges(prefabWithLegacyMaterials.m_templateId);

            // Request source control to edit prefab file
            AzToolsFramework::SourceControlCommandBus::Broadcast(
                &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
                prefabWithLegacyMaterials.m_prefabFullPath.c_str(), true,
                [prefabWithLegacyMaterials]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    // This is called from the main thread on the next frame from TickBus,
                    // that is why 'prefabWithLegacyMaterials' is captured as a copy.
                    if (!info.IsReadOnly())
                    {
                        auto* prefabLoader = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
                        if (!prefabLoader->SaveTemplate(prefabWithLegacyMaterials.m_templateId))
                        {
                            AZ_Warning("PhysXMaterialConversion", false, "Unable to save prefab '%s'",
                                prefabWithLegacyMaterials.m_prefabFullPath.c_str());
                        }
                    }
                    else
                    {
                        AZ_Warning("PhysXMaterialConversion", false, "Unable to check out asset '%s' in source control.",
                            prefabWithLegacyMaterials.m_prefabFullPath.c_str());
                    }
                }
            );
        }
        else
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No changes were done to the prefab.\n");
        }

        AZ_TracePrintf("PhysXMaterialConversion", "\n");
    }

    void FixAssetsUsingPhysicsLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        bool prefabSystemEnabled = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
        if (!prefabSystemEnabled)
        {
            AZ_Error("PhysXMaterialConversion", false, "Prefabs system is not enabled.");
            return;
        }

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for prefabs with legacy physics material assets...\n");
        AZStd::vector<PrefabInfo> prefabsWithLegacyMaterials = CollectPrefabsWithLegacyMaterials();
        if (prefabsWithLegacyMaterials.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No prefabs found that contain legacy physics materials.\n");
            return;
        }
        AZ_TracePrintf(
            "PhysXMaterialConversion", "Found %zu prefabs containing legacy physics materials.\n", prefabsWithLegacyMaterials.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for converted physics material assets...\n");
        LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap = CollectConvertedMaterialIds();
        if (legacyMaterialIdToNewAssetIdMap.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No converted physics material assets found.\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu converted physics materials.\n", legacyMaterialIdToNewAssetIdMap.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        for (auto& prefabWithLegacyMaterials : prefabsWithLegacyMaterials)
        {
            FixPrefabPhysicsMaterials(prefabWithLegacyMaterials, legacyMaterialIdToNewAssetIdMap);
        }
    }
} // namespace PhysX
