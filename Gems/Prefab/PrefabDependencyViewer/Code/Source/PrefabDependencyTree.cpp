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
    /* static */ Outcome PrefabDependencyTree::GenerateTreeAndSetRoot(TemplateId tid,
                    PrefabSystemComponentInterface* s_prefabSystemComponentInterface)
    {
        PrefabDependencyTree graph;

        const TemplateId InvalidTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;

        using pair = AZStd::pair<TemplateId, Utils::Node*>;
        AZStd::stack<pair> stack;
        stack.emplace(tid, nullptr);

        while (!stack.empty())
        {
            // Get the current TemplateId we are looking at and it's parent.
            pair tidAndParent = stack.top();
            stack.pop();

            TemplateId templateId = tidAndParent.first;

            if (InvalidTemplateId == templateId)
            {
                return AZ::Failure(AZStd::string_view("PrefabDependencyTreeGenerator - Invalid TemplateId found"));
            }

            Utils::Node* parent = tidAndParent.second;

            // Get the JSON for the current Template we are looking at.
            PrefabDom& prefabDom = s_prefabSystemComponentInterface->FindTemplateDom(templateId);

            // Get the source file of the current Template
            auto sourceIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
            if (sourceIterator == prefabDom.MemberEnd() || !sourceIterator->value.IsString())
            {
                return AZ::Failure(AZStd::string_view("PrefabDependencyTree - Source Attribute missing or it's not a String"));
            }

            auto&& source = sourceIterator->value;
            const char* sourceFileName = source.GetString();

            // Create a new node for the current Template and
            // Connect it to it's parent.
            NodePtr node = Utils::Node::CreatePrefabNode(templateId, sourceFileName);
            if (parent)
            {
                parent->AddChild(node);
            }
            else
            {
                // Only during the first iteration is the parent going to be nullptr
                // and only at this point graph should have a root. If we reach
                // this block and root is already set and then SetRoot would assert.
                graph.SetRoot(node);
            }
            AssetList assetList = GetAssets(prefabDom);
            AddAssetNodeToPrefab(prefabDom, node);

            // Go through current Template's nested instances
            // and put their TemplateId and current Template node
            // as their parent on the stack.
            auto instancesIterator = prefabDom.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::InstancesName);

            if (instancesIterator != prefabDom.MemberEnd())
            {
                auto&& instances = instancesIterator->value;

                if (instances.IsObject())
                {
                    for (auto&& instance : instances.GetObject())
                    {
                        // Get the source file of the current Template
                        sourceIterator = instance.value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
                        sourceFileName = "";
                        if (sourceIterator != instance.value.MemberEnd())
                        {
                            source = sourceIterator->value;
                            sourceFileName = source.IsString() ? source.GetString() : "";
                        }

                        // Checks for InvalidTemplateId when we pop the element off of the stack above.
                        TemplateId childtid = s_prefabSystemComponentInterface->GetTemplateIdFromFilePath(sourceFileName);
                        stack.emplace(childtid, node.get());
                    }
                }
            }
        }

        return AZ::Success(graph);
    }

    /* static */ AssetList PrefabDependencyTree::GetAssets(const PrefabDom& prefabDom)
    {
        AzToolsFramework::Prefab::Instance instance;
        AssetList referencedAssets;
        LoadInstanceFlags flags = LoadInstanceFlags::AssignRandomEntityId;

        bool result = AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(instance, prefabDom, referencedAssets, flags);

        AZ_Assert(result, "Prefab Dependency Viewer Gem - An error happened while loading the assets.");

        AZStd::vector<Utils::Node*> referencedAssetsNode;

        return referencedAssets;
    }

    /* static */ void PrefabDependencyTree::AddAssetNodeToPrefab(const PrefabDom& prefabDom, NodePtr node)
    {
        AssetList assetList = GetAssets(prefabDom);
        for (const auto& asset : assetList)
        {
            bool result;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, asset.GetId().m_guid, assetInfo,
                watchFolder);

            node->AddChild(Utils::Node::CreateAssetNode(assetInfo.m_relativePath));
        }
    }
} // namespace PrefabDependencyViewer
