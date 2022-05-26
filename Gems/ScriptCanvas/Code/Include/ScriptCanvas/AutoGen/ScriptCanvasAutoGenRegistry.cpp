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
        m_functions.clear();
    }

    AutoGenRegistry* AutoGenRegistry::GetInstance()
    {
        // Use static object so each module will keep its own registry collection
        static AutoGenRegistry s_autogenRegistry;

        return &s_autogenRegistry;
    }

    void AutoGenRegistry::Init()
    {
        auto registry = GetInstance();
        auto nodeRegistry = GetNodeRegistry();
        if (registry && nodeRegistry)
        {
            for (auto& iter : registry->m_nodeables)
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
            AZStd::string nodeableRegistry = AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenNodeableRegistrySuffix);
            auto nodeableIter = registry->m_nodeables.find(nodeableRegistry.c_str());
            if (nodeableIter != registry->m_nodeables.end())
            {
                nodeableIter->second->Init(nodeRegistry);
            }
        }
    }

    AZStd::vector<AZ::ComponentDescriptor*> AutoGenRegistry::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_nodeables)
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
            AZStd::string nodeableRegistry = AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenNodeableRegistrySuffix);
            auto nodeableIter = registry->m_nodeables.find(nodeableRegistry.c_str());
            if (nodeableIter != registry->m_nodeables.end())
            {
                auto nodeableDescriptors = nodeableIter->second->GetComponentDescriptors();
                descriptors.insert(descriptors.end(), nodeableDescriptors.begin(), nodeableDescriptors.end());
            }
        }
        return descriptors;
    }

    void AutoGenRegistry::Reflect(AZ::ReflectContext* context)
    {
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_functions)
            {
                if (iter.second)
                {
                    iter.second->Reflect(context);
                }
            }
            for (auto& iter : registry->m_nodeables)
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
            AZStd::string functionRegistry = AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenFunctionRegistrySuffix);
            auto functionIter = registry->m_functions.find(functionRegistry.c_str());
            if (functionIter != registry->m_functions.end())
            {
                functionIter->second->Reflect(context);
            }

            AZStd::string nodeableRegistry = AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenNodeableRegistrySuffix);
            auto nodeableIter = registry->m_nodeables.find(nodeableRegistry.c_str());
            if (nodeableIter != registry->m_nodeables.end())
            {
                nodeableIter->second->Reflect(context);
            }
        }
    }

    void AutoGenRegistry::RegisterFunction(const char* registryName, IScriptCanvasFunctionRegistry* registry)
    {
        if (m_functions.find(registryName) != m_functions.end())
        {
            AZ::Debug::Platform::OutputToDebugger(
                ScriptCanvasAutoGenRegistryName,
                AZStd::fixed_string<MaxMessageLength>::format(ScriptCanvasAutoGenRegistrationWarningMessage, registryName).c_str());
        }
        else if (registry != nullptr)
        {
            m_functions.emplace(registryName, registry);
        }
    }

    void AutoGenRegistry::UnregisterFunction(const char* registryName)
    {
        m_functions.erase(registryName);
    }

    void AutoGenRegistry::RegisterNodeable(const char* registryName, IScriptCanvasNodeableRegistry* registry)
    {
        if (m_nodeables.find(registryName) != m_nodeables.end())
        {
            AZ::Debug::Platform::OutputToDebugger(
                ScriptCanvasAutoGenRegistryName,
                AZStd::fixed_string<MaxMessageLength>::format(ScriptCanvasAutoGenRegistrationWarningMessage, registryName).c_str());
        }
        else if (registry != nullptr)
        {
            m_nodeables.emplace(registryName, registry);
        }
    }

    void AutoGenRegistry::UnregisterNodeable(const char* registryName)
    {
        m_nodeables.erase(registryName);
    }
} // namespace ScriptCanvas
