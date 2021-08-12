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

            /*
            // Setup for valid nested Prefab
            CreatePrefabAddSourceAndValue("ValidPrefab",   "Prefabs/ValidPrefab.prefab");
            CreatePrefabAddSourceAndValue("level11Prefab", "Prefabs/level11.prefab");
            CreatePrefabAddSourceAndValue("level12Prefab", "Prefabs/level12.prefab");
            CreatePrefabAddSourceAndValue("level13Prefab", "Prefabs/level13.prefab");
            CreatePrefabAddSourceAndValue("level22Prefab", "Prefabs/level22.prefab");
            CreatePrefabAddSourceAndValue("level23Prefab", "Prefabs/level23.prefab");
            CreatePrefabAddSourceAndValue("level31Prefab", "Prefabs/level31.prefab");

            // Level 1 setup
            AddInstance("ValidPrefab", "level11Prefab");
            AddInstance("ValidPrefab", "level12Prefab");
            AddInstance("ValidPrefab", "level13Prefab");

            // Level 2 setup
            AddInstance("level11Prefab", "level12Prefab");
            AddInstance("level13Prefab", "level22Prefab");
            AddInstance("level13Prefab", "level23Prefab");

            // Level 3 setup
            AddInstance("level23Prefab", "level31Prefab");
            */
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
            rapidjson::Value sourceKey(sourceName, allocator);
            rapidjson::Value sourceValue(prefabSource, allocator);

            prefabDom.AddMember(sourceKey, sourceValue, allocator);
        }

        void AddEntities(rapidjson::Value& prefabDom, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            rapidjson::Value entitiesKey(entitiesName, allocator);
            rapidjson::Value entitiesValue;
            entitiesValue.SetObject();

            prefabDom.AddMember(entitiesKey, entitiesValue, allocator);
        }

        void AddInstances(rapidjson::Value& prefabDom, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            rapidjson::Value instancesKey(instancesName, allocator);
            rapidjson::Value instancesValue;
            instancesValue.SetObject();

            prefabDom.AddMember(instancesKey, instancesValue, allocator);
        }

        void AddInstance(rapidjson::Value& root, const char* childSource,
                        rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            AZStd::string instanceAlias = AddInstanceWithoutSource(root, allocator);

            rapidjson::Value& instances = root.FindMember(instancesName)->value;

            rapidjson::Value& nestedInstanceAliasValue = instances[instanceAlias.c_str()];

            AddSourceEntitiesInstances(nestedInstanceAliasValue, childSource, allocator);
        }

        AZStd::string AddInstanceWithoutSource(rapidjson::Value& root, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            // Create an Instance Alias Name
            rapidjson::Value nestedInstanceAliasKey = CreateInstanceAlias(allocator);
            AZStd::string instanceAlias = nestedInstanceAliasKey.GetString();

            // Create the Instance alias value with source inside of it.
            rapidjson::Value nestedInstanceAliasValue;
            nestedInstanceAliasValue.SetObject();

            root[instancesName].AddMember(nestedInstanceAliasKey, nestedInstanceAliasValue, allocator);

            return instanceAlias;
        }

        rapidjson::Value CreateInstanceAlias(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator)
        {
            return CreateAlias(allocator, "Instance");
        }

        rapidjson::Value CreateAlias(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& allocator, AZStd::string type)
        {
            static int counter = 0;

            rapidjson::Value alias((type + "_" + AZStd::to_string(counter)).c_str(), allocator);

            ++counter;
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
        const char* sourceName = AzToolsFramework::Prefab::PrefabDomUtils::SourceName;
        const char* instancesName = AzToolsFramework::Prefab::PrefabDomUtils::InstancesName;
        const char* entitiesName = AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName;
    };
} // namespace PrefabDependencyViewer
