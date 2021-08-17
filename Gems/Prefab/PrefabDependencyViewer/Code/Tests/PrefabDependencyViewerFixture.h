/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <MockPrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace PrefabDependencyViewer
{
    using PrefabDom    = AzToolsFramework::Prefab::PrefabDom;
    using PrefabDomMap = AZStd::unordered_map<AZStd::string, PrefabDom>;
    using NodeList      = AZStd::vector<Utils::Node*>;

    struct PrefabDependencyViewerFixture : public UnitTest::ScopedAllocatorSetupFixture
    {
        PrefabDependencyViewerFixture()
            : m_prefabDomsCases(PrefabDomMap())
            , m_prefabSystemComponent(aznew MockPrefabSystemComponent())
            {}

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            // Setup for an invalid empty prefab
            m_prefabDomsCases["emptyJSON"] = CreateEmptyPrefabDom();

            
            // Setup for a root level Prefab with only Source Attribute
            m_prefabDomsCases["emptyJSONWithSource"] = CreatePrefabDom("Prefabs/emptyJSONWithSource.prefab");

            
            // Setup for root level Prefab with nested instances but one of the nested instances is missing source
            m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"] = CreatePrefabDom("Prefabs/Root.prefab");
            
            rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& invalidCaseAllocator =
                m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"].GetAllocator();

            AddInstance(m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"], "Prefabs/goodPrefab.prefab", invalidCaseAllocator);

            AddInstanceWithoutSource(m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"], invalidCaseAllocator);

            m_prefabDomsCases["ValidPrefab"] = CreateValidNestedPrefabsWithoutAssets();
        }

        PrefabDom CreateEmptyPrefabDom()
        {
            PrefabDom emptyDom = PrefabDom();
            emptyDom.SetObject();

            return emptyDom;
        }

        PrefabDom CreatePrefabDom(const char* prefabSource)
        {
            PrefabDom rootPrefabDom = CreateEmptyPrefabDom();

            auto& allocator = rootPrefabDom.GetAllocator();

            AddSourceEntitiesInstances(rootPrefabDom, prefabSource, allocator);

            return rootPrefabDom;
        }

        PrefabDom CreateValidNestedPrefabsWithoutAssets()
        {
            // Level 0 setup
            PrefabDom validNestedPrefabDom = CreatePrefabDom("Prefabs/ValidPrefab.prefab");
            auto& allocator = validNestedPrefabDom.GetAllocator();

            // Level 1 setup
            AZStd::string alias11 = AddInstance(validNestedPrefabDom, "Prefabs/level11.prefab", allocator);
            AZStd::string alias12 = AddInstance(validNestedPrefabDom, "Prefabs/level12.prefab", allocator);
            AZStd::string alias13 = AddInstance(validNestedPrefabDom, "Prefabs/level13.prefab", allocator);

            rapidjson::Value& level1instances = validNestedPrefabDom[m_instancesName];
            rapidjson::Value& level11PrefabDom = level1instances[alias11.c_str()];
            rapidjson::Value& level13PrefabDom = level1instances[alias13.c_str()];

            // Level 2 setup
            AZStd::string alias21 = AddInstance(level11PrefabDom, "Prefabs/level12.prefab", allocator);
            AZStd::string alias22 = AddInstance(level13PrefabDom, "Prefabs/level22.prefab", allocator);
            AZStd::string alias23 = AddInstance(level13PrefabDom, "Prefabs/level23.prefab", allocator);

            rapidjson::Value& level2instances = level13PrefabDom[m_instancesName];
            rapidjson::Value& level23PrefabDom = level2instances[alias23.c_str()];

            // Level 3 setup
            AddInstance(level23PrefabDom, "Prefabs/level31.prefab", allocator);

            return validNestedPrefabDom;
        }

        void AddSourceEntitiesInstances(rapidjson::Value& prefabDom, const char* prefabSource,
                            rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            AddSource(prefabDom, prefabSource, allocator);
            AddEntities(prefabDom, allocator);
            AddInstances(prefabDom, allocator);
        }

        void AddSource(rapidjson::Value& prefabDom, const char* prefabSource,
                rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            rapidjson::Value sourceKey(m_sourceName, allocator);
            rapidjson::Value sourceValue(prefabSource, allocator);

            prefabDom.AddMember(sourceKey, sourceValue, allocator);
        }

        void AddEntities(rapidjson::Value& prefabDom, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            rapidjson::Value entitiesKey(m_entitiesName, allocator);
            rapidjson::Value entitiesValue;
            entitiesValue.SetObject();

            prefabDom.AddMember(entitiesKey, entitiesValue, allocator);
        }

        void AddInstances(rapidjson::Value& prefabDom, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            rapidjson::Value instancesKey(m_instancesName, allocator);
            rapidjson::Value instancesValue;
            instancesValue.SetObject();

            prefabDom.AddMember(instancesKey, instancesValue, allocator);
        }

       AZStd::string AddInstance(rapidjson::Value& root, const char* childSource,
                        rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            AZStd::string instanceAlias = AddInstanceWithoutSource(root, allocator);

            rapidjson::Value& instances = root.FindMember(m_instancesName)->value;

            rapidjson::Value& nestedInstanceAliasValue = instances[instanceAlias.c_str()];

            AddSourceEntitiesInstances(nestedInstanceAliasValue, childSource, allocator);

            return instanceAlias;
        }

        AZStd::string AddInstanceWithoutSource(rapidjson::Value& root, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            // Create an Instance Alias Name
            rapidjson::Value nestedInstanceAliasKey = CreateInstanceAlias(allocator);
            AZStd::string instanceAlias = nestedInstanceAliasKey.GetString();

            // Create the Instance alias value with source inside of it.
            rapidjson::Value nestedInstanceAliasValue;
            nestedInstanceAliasValue.SetObject();

            root[m_instancesName].AddMember(nestedInstanceAliasKey, nestedInstanceAliasValue, allocator);

            return instanceAlias;
        }

        rapidjson::Value CreateInstanceAlias(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            return CreateAlias(allocator, "Instance");
        }

        rapidjson::Value CreateAlias(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator, AZStd::string type)
        {
            rapidjson::Value alias((type + "_" + AZStd::to_string(m_counter)).c_str(), allocator);

            ++m_counter;
            return alias;
        }

        void TearDown() override
        {
            delete m_prefabSystemComponent;
        }

        Utils::ChildrenList FindNodes(Utils::ChildrenList& nodeList, const char* source)
        {
            Utils::ChildrenList nodes;

            for (Utils::NodePtr node : nodeList)
            {
                if (node->GetMetaData()->GetDisplayName() == source)
                {
                    nodes.push_back(node);
                }
            }
            return nodes;
        }

        PrefabDomMap m_prefabDomsCases;
        MockPrefabSystemComponent* m_prefabSystemComponent;

        const TemplateId InvalidTemplateId = AzToolsFramework::Prefab::InvalidTemplateId;
        const char* m_sourceName = AzToolsFramework::Prefab::PrefabDomUtils::SourceName;
        const char* m_instancesName = AzToolsFramework::Prefab::PrefabDomUtils::InstancesName;
        const char* m_entitiesName = AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName;

        int m_counter = 0;
    };
} // namespace PrefabDependencyViewer
