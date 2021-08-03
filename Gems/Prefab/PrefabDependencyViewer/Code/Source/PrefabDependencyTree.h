/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/utils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

#include <DirectedGraph.h>

namespace PrefabDependencyViewer
{
    class PrefabDependencyTree;

    using TemplateId                     = AzToolsFramework::Prefab::TemplateId;
    using PrefabDom                      = AzToolsFramework::Prefab::PrefabDom;
    using PrefabSystemComponentInterface = AzToolsFramework::Prefab::PrefabSystemComponentInterface;
    using Outcome                        = AZ::Outcome<PrefabDependencyTree, const char*>;
    using AssetList                      = AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>;
    using LoadInstanceFlags              = AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFlags;
    using NodePtr                        = AZStd::shared_ptr<Utils::Node>;

    class PrefabDependencyTree : public Utils::DirectedTree
    {
    public:
        static Outcome GenerateTreeAndSetRoot(TemplateId tid,
            PrefabSystemComponentInterface* s_prefabSystemComponentInterface);

        static void AddAssetNodeToPrefab(const PrefabDom& prefabDom, NodePtr node);
        static AssetList GetAssets(const PrefabDom& prefabDom);
    };
}
