/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/fixed_string.h>
#include <ScriptCanvas/Libraries/ScriptCanvasCustomLibrary.h>

#include "ScriptCanvasAutoGenRegistry.h"

namespace ScriptCanvas
{
    static constexpr const char ScriptCanvasAutoGenFunctionRegistrySuffix[] = "FunctionRegistry";
    static constexpr const char ScriptCanvasAutoGenNodeableRegistrySuffix[] = "NodeableRegistry";
    static constexpr const char ScriptCanvasAutoGenRegistryName[] = "AutoGenRegistry";
    static constexpr int MaxMessageLength = 4096;
    static constexpr const char ScriptCanvasAutoGenRegistrationWarningMessage[] = "[Warning] Registry name %s is occupied already, ignore AutoGen registry registration.\n";

    AutoGenRegistry::~AutoGenRegistry()
    {
        m_registries.clear();
    }

    AutoGenRegistry* AutoGenRegistry::GetInstance()
    {
        // Use static object so each module will keep its own registry collection
        static AutoGenRegistry s_autogenRegistry;

        return &s_autogenRegistry;
    }

    AZStd::vector<std::string> AutoGenRegistry::GetRegistryNames(const char* registryName)
    {
        AZStd::vector<std::string> result;
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenFunctionRegistrySuffix).c_str());
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenNodeableRegistrySuffix).c_str());
        return result;
    }

    void AutoGenRegistry::Init()
    {
        auto registry = GetInstance();
        auto nodeRegistry = GetNodeRegistry();
        if (registry && nodeRegistry)
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    iter.second->Init(nodeRegistry);
                }
            }
        }
    }

    void AutoGenRegistry::Init(const char* registryName)
    {
        auto registry = GetInstance();
        auto nodeRegistry = GetNodeRegistry();
        if (registry && nodeRegistry)
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name);
                if (registryIter != registry->m_registries.end())
                {
                    registryIter->second->Init(nodeRegistry);
                }
            }
        }
    }

    AZStd::vector<AZ::ComponentDescriptor*> AutoGenRegistry::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    auto nodeableDescriptors = iter.second->GetComponentDescriptors();
                    descriptors.insert(descriptors.end(), nodeableDescriptors.begin(), nodeableDescriptors.end());
                }
            }
        }
        return descriptors;
    }

    AZStd::vector<AZ::ComponentDescriptor*> AutoGenRegistry::GetComponentDescriptors(const char* registryName)
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;
        if (auto registry = GetInstance())
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name);
                if (registryIter != registry->m_registries.end())
                {
                    auto registryDescriptors = registryIter->second->GetComponentDescriptors();
                    descriptors.insert(descriptors.end(), registryDescriptors.begin(), registryDescriptors.end());
                }
            }
        }
        return descriptors;
    }

    void AutoGenRegistry::Reflect(AZ::ReflectContext* context)
    {
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    iter.second->Reflect(context);
                }
            }
        }
    }

    void AutoGenRegistry::Reflect(AZ::ReflectContext* context, const char* registryName)
    {
        if (auto registry = GetInstance())
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name);
                if (registryIter != registry->m_registries.end())
                {
                    registryIter->second->Reflect(context);
                }
            }
        }
    }

    void AutoGenRegistry::RegisterRegistry(const char* registryName, IScriptCanvasRegistry* registry)
    {
        if (m_registries.find(registryName) != m_registries.end())
        {
            AZ::Debug::Platform::OutputToDebugger(
                ScriptCanvasAutoGenRegistryName,
                AZStd::fixed_string<MaxMessageLength>::format(ScriptCanvasAutoGenRegistrationWarningMessage, registryName).c_str());
        }
        else if (registry != nullptr)
        {
            m_registries.emplace(registryName, registry);
        }
    }

    void AutoGenRegistry::UnregisterRegistry(const char* registryName)
    {
        m_registries.erase(registryName);
    }
} // namespace ScriptCanvas
