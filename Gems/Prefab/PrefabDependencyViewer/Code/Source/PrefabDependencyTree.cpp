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
            // Create a deep copy of the root Prefab Dom so the Template
            // can be modified in the GenerateTreeAndSetRootRecursive method.
            rapidjson::Value& rootPrefabDom = static_cast<rapidjson::Value&>(
                                                prefabSystemComponentInterface->FindTemplateDom(tid));

            AssetDescriptionCountMap rootParentAssetCount;

            NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(rootPrefabDom, rootParentAssetCount);
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
                                    const rapidjson::Value& prefabDom, AssetDescriptionCountMap& parentCount)
    {
        // Get all the nested assets and their count
        AssetList currentAssets = GetAssets(prefabDom);
        AssetDescriptionCountMap currentCount = GetAssetsDescriptionCountMap(currentAssets);

        // Remove the children asset count from the parent count
        DecreaseParentAssetCount(parentCount, currentCount);


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
                    NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(instance.value, currentCount);
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
        AddAssetNodeToPrefab(currentAssets, currentNode, currentCount);
        return AZ::Success(currentNode);
    }

    /* static */ void PrefabDependencyTree::AddAssetNodeToPrefab(const AssetList& assetList, NodePtr node,
                                                        AssetDescriptionCountMap& assetDescriptionCountMap)
    {
        // No need to show multiple nodes for the same source asset
        // with multiple product assets that spawn out of it.
        // Instead keep track of their count and display that in the node.
        AZStd::unordered_map<AZStd::string, int> sourceAssetCount;

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

                AZStd::string assetDescription = assetInfo.m_relativePath + productAssetDesciption;

                // If all the children claimed the asset, then the asset description count should
                // go to 0 which implies that the current asset is not a node dependency.
                auto assetDescriptionCountIterator = assetDescriptionCountMap.find(assetDescription);
                if (assetDescriptionCountIterator != assetDescriptionCountMap.end() && assetDescriptionCountIterator->second > 0)
                {
                    // Only want to add an asset to the sourceAssetCount if
                    // its assetDescriptionCount is non zero.
                    auto sourceAssetCountIterator = sourceAssetCount.find(assetInfo.m_relativePath);
                    if (sourceAssetCountIterator == sourceAssetCount.end())
                    {
                        sourceAssetCount[assetInfo.m_relativePath] = 1;
                    }
                    else
                    {
                        sourceAssetCountIterator->second += 1;
                    }

                    // Update the current asset's description count.
                    assetDescriptionCountIterator->second -= 1;
                }
            }
        }

        // Create nodes with the Source Asset Name and it's product dependency count.
        for (const AZStd::pair<AZStd::string, int>& sourceCount : sourceAssetCount) {
            AZStd::string assetDescription = sourceCount.first + " (" + AZStd::to_string(sourceCount.second) + ")";
            node->AddChild(Utils::Node::CreateAssetNode(assetDescription));
        }
    }

    /* static */ AssetList PrefabDependencyTree::GetAssets(const rapidjson::Value& prefabDom)
    {
        AzToolsFramework::Prefab::Instance instance;
        AssetList referencedAssets;
        LoadInstanceFlags flags = LoadInstanceFlags::AssignRandomEntityId;

        bool result = LoadInstanceFromPrefabDom(instance, prefabDom, referencedAssets, flags);

        AZ_Error("Prefab Dependency Viewer", result,  "An error happened while loading the prefab data(Assets). "
                                                      "Check the log output for errors, and the prefab "
                                                      "files for corruption.");

        return referencedAssets;
    }

    /* static */ AssetDescriptionCountMap PrefabDependencyTree::GetAssetsDescriptionCountMap(AssetList allNestedAssets)
    {
        AssetDescriptionCountMap count;

        for (const auto& asset : allNestedAssets)
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
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    productAssetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, asset.GetId());

                AZStd::string productAssetDesciption = productAssetInfo.m_relativePath.empty() ? "" : ": " + productAssetInfo.m_relativePath;

                AZStd::string assetDescription = assetInfo.m_relativePath + productAssetDesciption;

                // If all the children claimed the asset, then the asset description count should
                // go to 0 which implies that the current asset is not a node dependency.
                auto it = count.find(assetDescription);
                if (it == count.end())
                {
                    count[assetDescription] = 1;
                }
                else
                {
                    it->second += 1;
                }
            }
        }

        return count;
    }

    /* static */ void PrefabDependencyTree::DecreaseParentAssetCount(
                               AssetDescriptionCountMap& parentCount,
                               const AssetDescriptionCountMap& childCount)
    {
        if (parentCount.size() == 0)
        {
            return;
        }

        for (const auto& [assetDescription, count] : childCount)
        {
            auto it = parentCount.find(assetDescription);
            if (it != parentCount.end())
            {
                it->second -= count;
            }
        }
    }

    /* static */ bool PrefabDependencyTree::LoadInstanceFromPrefabDom(
                Instance& instance, const rapidjson::Value& prefabDom,
                AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets,
                LoadInstanceFlags flags)
    {
        // When entities are rebuilt they are first destroyed. As a result any assets they were exclusively holding on to will
        // be released and reloaded once the entities are built up again. By suspending asset release temporarily the asset reload
        // is avoided.
        AZ::Data::AssetManager::Instance().SuspendAssetRelease();

        InstanceEntityIdMapper entityIdMapper;
        entityIdMapper.SetLoadingInstance(instance);
        if ((flags & LoadInstanceFlags::AssignRandomEntityId) == LoadInstanceFlags::AssignRandomEntityId)
        {
            entityIdMapper.SetEntityIdGenerationApproach(InstanceEntityIdMapper::EntityIdGenerationApproach::Random);
        }

        AZ::JsonDeserializerSettings settings;
        // The InstanceEntityIdMapper is registered twice because it's used in several places during deserialization where one is
        // specific for the InstanceEntityIdMapper and once for the generic JsonEntityIdMapper. Because the Json Serializer's meta
        // data has strict typing and doesn't look for inheritance both have to be explicitly added so they're found both locations.
        settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));
        settings.m_metadata.Add(&entityIdMapper);
        settings.m_metadata.Create<AZ::Data::SerializedAssetTracker>();

        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(instance, prefabDom, settings);

        AZ::Data::AssetManager::Instance().ResumeAssetRelease();

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error(
                "Prefab", false,
                "Failed to de-serialize Prefab Instance from Prefab DOM. "
                "Unable to proceed.");

            return false;
        }
        AZ::Data::SerializedAssetTracker* assetTracker = settings.m_metadata.Find<AZ::Data::SerializedAssetTracker>();

        referencedAssets = AZStd::move(assetTracker->GetTrackedAssets());
        return true;
    }
} // namespace PrefabDependencyViewer
