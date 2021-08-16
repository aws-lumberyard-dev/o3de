/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabDependencyTree.h>

namespace PrefabDependencyViewer
{
    /* static */ TreeOutcome PrefabDependencyTree::GenerateTreeAndSetRoot(TemplateId tid,
                            PrefabSystemComponentInterface* prefabSystemComponentInterface)
    {
        if (AzToolsFramework::Prefab::InvalidTemplateId == tid)
        {
            return AZ::Failure(AZStd::string_view("PrefabDependencyTreeGenerator - Invalid root TemplateId found."));
        }
        else
        {
            PrefabDom& rootPrefabDom = prefabSystemComponentInterface->FindTemplateDom(tid);

            NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(rootPrefabDom);
            if (outcome.IsSuccess())
            {
                PrefabDependencyTree graph;
                graph.SetRoot(outcome.GetValue());

                return graph;
            }
            else
            {
                return AZ::Failure(outcome.GetError());
            }
        }
    }

    /* static */ NodePtrOutcome PrefabDependencyTree::GenerateTreeAndSetRootRecursive(
                                                    const rapidjson::Value& prefabDom)
    {
        // Get all the nested assets and their count
        AssetList currentAssets = GetAssets(prefabDom);

        // Get the source file of the current Template
        auto sourceIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
        if (sourceIterator == prefabDom.MemberEnd() || !sourceIterator->value.IsString())
        {
            return AZ::Failure(AZStd::string_view("PrefabDependencyTree - Source Attribute missing or it's not a String"));
        }

        auto&& source = sourceIterator->value;
        const char* sourceFileName = source.GetString();

        // Create a new node for the current Template.
        NodePtr currentNode = Utils::Node::CreatePrefabNode(sourceFileName);

        // Go through current Template's nested instances.
        // Get and recurse on their PrefabDoms. If successful,
        // then add the child Node pointer to the parent.
        // Otherwise, return the error output immediately.
        auto instancesIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::InstancesName);

        if (instancesIterator != prefabDom.MemberEnd())
        {
            auto&& instances = instancesIterator->value;

            if (instances.IsObject())
            {
                for (auto&& instance : instances.GetObject())
                {
                    // Recurse on the nested instance.
                    NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(instance.value);
                    if (outcome.IsSuccess())
                    {
                        currentNode->AddChild(outcome.GetValue());
                    }
                    else
                    {
                        return AZ::Failure(outcome.GetError());
                    }
                }
            }
        }

        // Add assets to the PrefabNode
        AddAssetNodeToPrefab(currentAssets, currentNode);
        return AZ::Success(currentNode);
    }

    /* static */ void PrefabDependencyTree::AddAssetNodeToPrefab(const AssetList& assetList, NodePtr node)
    {
        // No need to show multiple nodes for the same source asset
        // with multiple product assets that spawn out of it.
        // Instead keep track of their count and display that in the node.
        AZStd::unordered_map<AZStd::string, int> sourceAssetCountMap;

        for (const auto& asset : assetList)
        {
            bool result;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, asset.GetId().m_guid, assetInfo,
                watchFolder);

            // Unassigned default assets would have an empty source and
            // therefore are an invalid asset.
            if (!assetInfo.m_relativePath.empty())
            {
                AZ::Data::AssetInfo productAssetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(productAssetInfo,
                    &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());

                AZStd::string productAssetDesciption =
                    productAssetInfo.m_relativePath == "" ? "" : ": " + productAssetInfo.m_relativePath;

                auto sourceAssetCountIterator = sourceAssetCountMap.find(assetInfo.m_relativePath);
                if (sourceAssetCountIterator == sourceAssetCountMap.end())
                {
                    sourceAssetCountMap.emplace(assetInfo.m_relativePath, 1);
                }
                else
                {
                    sourceAssetCountIterator->second += 1;
                }
            }
        }

        // Create nodes with the Source Asset Name and it's product dependency count.
        for (const AZStd::pair<AZStd::string, int>& sourceCount : sourceAssetCountMap) {
            AZStd::string assetDescription = sourceCount.first + " (" + AZStd::to_string(sourceCount.second) + ")";
            node->AddChild(Utils::Node::CreateAssetNode(assetDescription));
        }
    }

    /* static */ AssetList PrefabDependencyTree::GetAssets(const rapidjson::Value& prefabDom)
    {
        AssetList referencedAssets;
        bool result = LoadAssetsFromEntities(prefabDom, referencedAssets);

        AZ_Error("Prefab Dependency Viewer", result,  "An error happened while loading the Assets."
                                                      "Check the log output for errors, and the prefab "
                                                      "files for corruption.");

        return referencedAssets;
    }

    /* static */ bool PrefabDependencyTree::LoadAssetsFromEntities(const rapidjson::Value& prefabDom, AssetList& referencedAssets)
    {
        auto entitiesIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);

        if (entitiesIterator != prefabDom.MemberEnd())
        {
            auto&& entities = entitiesIterator->value;

            if (entities.IsObject())
            {
                for (auto&& entityDom : entities.GetObject())
                {
                    AZ::Entity entity("");
                    bool result = LoadAssetsFromEntity(entity, entityDom.value, referencedAssets);
                    if (!result)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    /* static */ bool PrefabDependencyTree::LoadAssetsFromEntity(AZ::Entity& entity, const rapidjson::Value& entityDom,
                                                                    AssetList& referencedAssets)
    {
        AZ::Data::AssetManager::Instance().SuspendAssetRelease();

        AZ::JsonDeserializerSettings settings;

        settings.m_metadata.Create<AZ::Data::SerializedAssetTracker>();

        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(entity, entityDom, settings);

        AZ::Data::AssetManager::Instance().ResumeAssetRelease();

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error(
                "Prefab", false,
                "Failed to de-serialize Entity from Entity DOM. "
                "Unable to proceed.");

            return false;
        }

        AZ::Data::SerializedAssetTracker* assetTracker = settings.m_metadata.Find<AZ::Data::SerializedAssetTracker>();

        AssetList& currentAssets = assetTracker->GetTrackedAssets();
        referencedAssets.insert(referencedAssets.end(),
                                AZStd::make_move_iterator(currentAssets.begin()),
                                AZStd::make_move_iterator(currentAssets.end()));

        return true;
    }
} // namespace PrefabDependencyViewer
