/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetJsonSerializer.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/utils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

#include <DirectedGraph.h>

namespace PrefabDependencyViewer
{
    class PrefabDependencyTree;

    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    
    using AssetList                      = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
    using AssetDescriptionCountMap       = AZStd::unordered_map<AZStd::string, int>;
    using LoadInstanceFlags              = AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFlags;

    using NodePtr                        = AZStd::shared_ptr<Utils::Node>;
    using TreeOutcome                    = AZ::Outcome<PrefabDependencyTree, AZStd::string_view>;
    using NodePtrOutcome                 = AZ::Outcome<NodePtr, AZStd::string_view>;

    using Instance                       = AzToolsFramework::Prefab::Instance;
    using InstanceEntityIdMapper         = AzToolsFramework::Prefab::InstanceEntityIdMapper;

    class PrefabDependencyTree : public Utils::DirectedTree
    {
    public:
        static TreeOutcome GenerateTreeAndSetRoot(TemplateId tid,
                               PrefabSystemComponentInterface* s_prefabSystemComponentInterface);

    private:
        static NodePtrOutcome GenerateTreeAndSetRootRecursive(const rapidjson::Value& prefabDom,
                                                  AssetDescriptionCountMap& parentAssetCountMap);

        static AssetDescriptionCountMap GetAssetsDescriptionCountMap(AssetList allNestedAssets);

        static void AddAssetNodeToPrefab(const AssetList& assetList, NodePtr node,
                                            AssetDescriptionCountMap& assetDescriptionCountMap);

        static AssetList GetAssets(const rapidjson::Value& prefabDom);

        static void DecreaseParentAssetCount(AssetDescriptionCountMap& parentAssetCountMap,
                                        const AssetDescriptionCountMap& childAssetCountMap);

        static bool LoadInstanceFromPrefabDom(
            Instance& instance, const rapidjson::Value& prefabDom,
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& referencedAssets,
            LoadInstanceFlags flags);
    };
}
