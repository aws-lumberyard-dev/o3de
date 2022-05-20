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
#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>

namespace PhysX
{
    void FixPrefabsWithPhysicsLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC(
        "ed_physxFixPrefabsWithPhysicsLegacyMaterials",
        FixPrefabsWithPhysicsLegacyMaterials,
        AZ::ConsoleFunctorFlags::Null,
        "Finds prefabs that contain components using legacy physics material ids and fixes them by using new physx material assets.");

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

    rapidjson::Document::MemberIterator FindMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        auto memberEnd = prefabComponent.MemberEnd();

        if (memberChain.empty())
        {
            return memberEnd;
        }

        auto memberIter = prefabComponent.FindMember(memberChain[0].c_str());
        if (memberIter == prefabComponent.MemberEnd())
        {
            return memberEnd;
        }

        for (size_t i = 1; i < memberChain.size(); ++i)
        {
            memberIter = memberIter->value.FindMember(memberChain[i].c_str());
            if (memberIter == memberIter->value.MemberEnd())
            {
                return memberEnd;
            }
        }
        return memberIter;
    }

    rapidjson::Document::ConstMemberIterator FindConstMemberChainInPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent)
    {
        return FindMemberChainInPrefabComponent(memberChain, const_cast<AzToolsFramework::Prefab::PrefabDomValue&>(prefabComponent));
    }

    template<class T>
    bool LoadObjectFromPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain, const AzToolsFramework::Prefab::PrefabDomValue& prefabComponent, T& object)
    {
        auto memberIter = FindConstMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (memberIter == prefabComponent.MemberEnd())
        {
            return false;
        }

        auto result = AZ::JsonSerialization::Load(&object, azrtti_typeid<T>(), memberIter->value);

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    template<class T>
    bool StoreObjectToPrefabComponent(
        const AZStd::vector<AZStd::string>& memberChain,
        AzToolsFramework::Prefab::PrefabDom& prefabDom,
        AzToolsFramework::Prefab::PrefabDomValue& prefabComponent,
        const T& object)
    {
        auto memberIter = FindMemberChainInPrefabComponent(memberChain, prefabComponent);
        if (memberIter == prefabComponent.MemberEnd())
        {
            return false;
        }

        T defaultObject;

        auto result =
            AZ::JsonSerialization::Store(memberIter->value, prefabDom.GetAllocator(), &object, &defaultObject, azrtti_typeid<T>());

        return result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed;
    }

    bool PrefabHasLegacyMaterialId(AzToolsFramework::Prefab::PrefabDom& prefabDom)
    {
        for ([[maybe_unused]] auto* entity : GetPrefabEntities(prefabDom))
        {
            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorColliderComponent>(), *entity))
            {
                // New: "ColliderConfiguration" -> "MaterialSlots" -> "Slots" -> vector of MaterialSlot -> "MaterialAsset"
                // Old: "ColliderConfiguration" -> "MaterialSelection" -> "MaterialIds" -> vector of MaterialId -> "MaterialId"
                PhysicsLegacy::MaterialSelection materialSelection;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                        { "ColliderConfiguration", "MaterialSelection" }, *component, materialSelection))
                {
                    for (const auto& legacyMaterialId : materialSelection.m_materialIdsAssignedToSlots)
                    {
                        if (!legacyMaterialId.m_id.IsNull())
                        {
                            return true;
                        }
                    }
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorShapeColliderComponent>(), *entity))
            {
                // New: "ColliderConfiguration" -> "MaterialSlots" -> "Slots" -> vector of MaterialSlot -> "MaterialAsset"
                // Old: "ColliderConfiguration" -> "MaterialSelection" -> "MaterialIds" -> vector of MaterialId -> "MaterialId"
                PhysicsLegacy::MaterialSelection materialSelection;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                    { "ColliderConfiguration", "MaterialSelection" }, *component, materialSelection))
                {
                    for (const auto& legacyMaterialId : materialSelection.m_materialIdsAssignedToSlots)
                    {
                        if (!legacyMaterialId.m_id.IsNull())
                        {
                            return true;
                        }
                    }
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorHeightfieldColliderComponent>(), *entity))
            {
                // New: "ColliderConfiguration" -> "MaterialSlots" -> "Slots" -> vector of MaterialSlot -> "MaterialAsset"
                // Old: "ColliderConfiguration" -> "MaterialSelection" -> "MaterialIds" -> vector of MaterialId -> "MaterialId"
                PhysicsLegacy::MaterialSelection materialSelection;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                    { "ColliderConfiguration", "MaterialSelection" }, *component, materialSelection))
                {
                    for (const auto& legacyMaterialId : materialSelection.m_materialIdsAssignedToSlots)
                    {
                        if (!legacyMaterialId.m_id.IsNull())
                        {
                            return true;
                        }
                    }
                }
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorCharacterControllerComponent>(), *entity))
            {
                // New: "Configuration" -> "MaterialSlots" -> "Slots" -> vector of MaterialSlot -> "MaterialAsset"
                // Old: "Configuration" -> "Material" -> "MaterialIds" -> vector of MaterialId -> "MaterialId"
                PhysicsLegacy::MaterialSelection materialSelection;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialSelection>(
                    { "Configuration", "Material" }, *component, materialSelection))
                {
                    for (const auto& legacyMaterialId : materialSelection.m_materialIdsAssignedToSlots)
                    {
                        if (!legacyMaterialId.m_id.IsNull())
                        {
                            return true;
                        }
                    }
                }
            }

            // EditorBlastFamilyComponent
            for (auto* component : GetPrefabComponents(AZ::TypeId::CreateString("{ECB1689A-2B65-44D1-9227-9E62962A7FF7}"), *entity))
            {
                // New: "PhysicsMaterialAsset"
                // Old: "PhysicsMaterial" -> "MaterialId"
                PhysicsLegacy::MaterialId legacyMaterialId;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(
                    { "PhysicsMaterial" }, *component, legacyMaterialId))
                {
                    if (!legacyMaterialId.m_id.IsNull())
                    {
                        return true;
                    }
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
            if (assetInfo.m_assetType != EditorMaterialAsset::RTTI_Type())
            {
                return;
            }

            AZ::Data::Asset<EditorMaterialAsset> materialAsset(assetId, assetInfo.m_assetType);
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
                AZ_Warning("PhysXMaterialConversion", false, "Unable to load physx material asset '%s'.", assetInfo.m_relativePath.c_str());
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, assetEnumerationCB, nullptr);

        return legacyMaterialIdToNewAssetIdMap;
    }

    void FixPrefabPhysicsMaterials(
        [[maybe_unused]] PrefabInfo& prefabWithLegacyMaterials,
        [[maybe_unused]] const LegacyMaterialIdToNewAssetIdMap& legacyMaterialIdToNewAssetIdMap)
    {
        AZ_TracePrintf("PhysXMaterialConversion", "Fixing prefab '%s'.\n", prefabWithLegacyMaterials.m_prefabFullPath.c_str());

        bool prefabDomModified = false;

        for (auto* entity : GetPrefabEntities(prefabWithLegacyMaterials.m_template->GetPrefabDom()))
        {
            /*for (auto* component : GetPrefabComponents(azrtti_typeid<EditorColliderComponent>(), *entity))
            {
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorShapeColliderComponent>(), *entity))
            {
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorHeightfieldColliderComponent>(), *entity))
            {
            }

            for (auto* component : GetPrefabComponents(azrtti_typeid<EditorCharacterControllerComponent>(), *entity))
            {
            }*/

            // EditorBlastFamilyComponent
            for (auto* component : GetPrefabComponents(AZ::TypeId::CreateString("{ECB1689A-2B65-44D1-9227-9E62962A7FF7}"), *entity))
            {
                PhysicsLegacy::MaterialId legacyMaterialId;
                if (LoadObjectFromPrefabComponent<PhysicsLegacy::MaterialId>(
                    { "PhysicsMaterial" }, *component, legacyMaterialId))
                {
                    if (legacyMaterialId.m_id.IsNull())
                    {
                        continue;
                    }

                    auto it = legacyMaterialIdToNewAssetIdMap.find(legacyMaterialId.m_id);
                    if (it == legacyMaterialIdToNewAssetIdMap.end())
                    {
                        AZ_Warning("PhysXMaterialConversion", false, "Unable to find a physx material asset to replace legacy material id '%s' with.",
                            legacyMaterialId.m_id.ToString<AZStd::string>().c_str());
                        continue;
                    }

                    const AZ::Data::AssetId newMaterialAssetId = it->second;

                    AZStd::optional<AZStd::string> newMaterialAssetFullPath = GetFullSourceAssetPathById(newMaterialAssetId);
                    AZ_Assert(newMaterialAssetFullPath.has_value(), "Cannot get full source asset path from id %s",
                        newMaterialAssetId.ToString<AZStd::string>().c_str());

                    AZ_TracePrintf("PhysXMaterialConversion", "Legacy material id '%s' will be replaced by physx material asset '%s'.\n",
                        legacyMaterialId.m_id.ToString<AZStd::string>().c_str(),
                        newMaterialAssetFullPath->c_str());

                    // Remove legacy material id field
                    component->RemoveMember("PhysicsMaterial");

                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, newMaterialAssetId);
                    AZ::Data::Asset<EditorMaterialAsset> newMaterialAsset(newMaterialAssetId, assetInfo.m_assetType, assetInfo.m_relativePath);

                    if (!StoreObjectToPrefabComponent< AZ::Data::Asset<EditorMaterialAsset>>(
                        { "PhysicsMaterialAsset" }, prefabWithLegacyMaterials.m_template->GetPrefabDom(), *component, newMaterialAsset))
                    {
                        AZ_Warning("PhysXMaterialConversion", false, "Unable to set physx material asset to %s' in prefab.",
                            newMaterialAssetId.ToString<AZStd::string>().c_str());
                        continue;
                    }

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

    void FixPrefabsWithPhysicsLegacyMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
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

        AZ_TracePrintf("PhysXMaterialConversion", "Searching for converted physx material assets...\n");
        LegacyMaterialIdToNewAssetIdMap legacyMaterialIdToNewAssetIdMap = CollectConvertedMaterialIds();
        if (legacyMaterialIdToNewAssetIdMap.empty())
        {
            AZ_TracePrintf("PhysXMaterialConversion", "No converted physx material assets found.\n");
            return;
        }
        AZ_TracePrintf("PhysXMaterialConversion", "Found %zu converted physx materials.\n", legacyMaterialIdToNewAssetIdMap.size());
        AZ_TracePrintf("PhysXMaterialConversion", "\n");

        for (auto& prefabWithLegacyMaterials : prefabsWithLegacyMaterials)
        {
            FixPrefabPhysicsMaterials(prefabWithLegacyMaterials, legacyMaterialIdToNewAssetIdMap);
        }
    }
} // namespace PhysX
