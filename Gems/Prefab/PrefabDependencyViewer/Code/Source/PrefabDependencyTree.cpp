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

            AssetList allNestedAssets = GetAssets(rootPrefabDom);
            AssetDescriptionCountMap count = GetAssetsDescriptionCountMap(allNestedAssets);

            NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(tid, prefabSystemComponentInterface, count);
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

    /* static */ NodePtrOutcome PrefabDependencyTree::GenerateTreeAndSetRootRecursive(TemplateId templateId,
                                        PrefabSystemComponentInterface* prefabSystemComponentInterface,
                                        AssetDescriptionCountMap& count)
    {
        if (AzToolsFramework::Prefab::InvalidTemplateId == templateId)
        {
            return AZ::Failure(AZStd::string_view("PrefabDependencyTreeGenerator - Invalid TemplateId found."));
        }
        else
        {
            // Get the JSON for the current Template we are looking at.
            PrefabDom& prefabDom = prefabSystemComponentInterface->FindTemplateDom(templateId); 

            // Get the source file of the current Template
            auto sourceIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
            if (sourceIterator == prefabDom.MemberEnd() || !sourceIterator->value.IsString())
            {
                return AZ::Failure(AZStd::string_view("PrefabDependencyTree - Source Attribute missing or it's not a String"));
            }

            auto&& source = sourceIterator->value;
            const char* sourceFileName = source.GetString();

            // Create a new node for the current Template.
            NodePtr parent = Utils::Node::CreatePrefabNode(templateId, sourceFileName);

            // Go through current Template's nested instances.
            // Recurse on their Template Id's. If successful,
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
                        // Get the source file of the nested instance.
                        sourceIterator = instance.value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
                        sourceFileName = "";
                        if (sourceIterator != instance.value.MemberEnd())
                        {
                            source = sourceIterator->value;
                            sourceFileName = source.IsString() ? source.GetString() : "";
                        }

                        // Get the TemplateId for the nested Instance.
                        TemplateId childTemplateId = prefabSystemComponentInterface->GetTemplateIdFromFilePath(sourceFileName);

                        // Recurse on the nested instance.
                        NodePtrOutcome outcome = GenerateTreeAndSetRootRecursive(childTemplateId, prefabSystemComponentInterface, count);
                        if (outcome.IsSuccess())
                        {
                            parent->AddChild(outcome.GetValue());
                        }
                        else
                        {
                            return AZ::Failure(outcome.GetError());
                        }
                    }
                }
            }

            // Add assets to the PrefabNode
            AssetList assetList = GetAssets(prefabDom);
            AddAssetNodeToPrefab(prefabDom, parent, count);
            return AZ::Success(parent);
        }
    }

    /* static */ void PrefabDependencyTree::AddAssetNodeToPrefab(const PrefabDom& prefabDom, NodePtr node,
                                                        AssetDescriptionCountMap& assetDescriptionCountMap)
    {
        AssetList assetList = GetAssets(prefabDom);
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

    /* static */ AssetList PrefabDependencyTree::GetAssets(const PrefabDom& prefabDom)
    {
        AzToolsFramework::Prefab::Instance instance;
        AssetList referencedAssets;
        LoadInstanceFlags flags = LoadInstanceFlags::AssignRandomEntityId;

        bool result = AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(instance, prefabDom, referencedAssets, flags);

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
} // namespace PrefabDependencyViewer
