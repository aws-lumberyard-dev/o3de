/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <string>
#include <unordered_map>

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
}

//! Macros to self-register AutoGen node into ScriptCanvas
#define REGISTER_SCRIPTCANVAS_AUTOGEN_FUNCTION(LIBRARY)\
    static ScriptCanvas::LIBRARY##FunctionRegistry s_AutoGenFunctionRegistry;
#define REGISTER_SCRIPTCANVAS_AUTOGEN_NODEABLE(LIBRARY)\
    static ScriptCanvas::LIBRARY##NodeableRegistry s_AutoGenNodeableRegistry;

//! AutoGen registry util macros
#define INIT_SCRIPTCANVAS_AUTOGEN(LIBRARY)\
    ScriptCanvas::AutoGenRegistry::Init(#LIBRARY);
#define REFLECT_SCRIPTCANVAS_AUTOGEN(LIBRARY, CONTEXT)\
    ScriptCanvas::AutoGenRegistry::Reflect(CONTEXT, #LIBRARY);
#define GET_SCRIPTCANVAS_AUTOGEN_COMPONENT_DESCRIPTORS(LIBRARY)\
    ScriptCanvas::AutoGenRegistry::GetComponentDescriptors(#LIBRARY)

namespace ScriptCanvas
{
    struct NodeRegistry;

    class IScriptCanvasRegistry
    {
    public:
        virtual void Init(NodeRegistry* nodeRegistry) = 0;
        virtual void Reflect(AZ::ReflectContext* context) = 0;
        virtual AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors() = 0;
    };

    //! AutoGenRegistry
    //! The registry contains all autogen functions, nodeables and grammars which will be registered
    //! for ScriptCanvas
    class AutoGenRegistry
    {
    public:
        AutoGenRegistry() = default;
        ~AutoGenRegistry();

        static AutoGenRegistry* GetInstance();

        //! Init all AutoGen registries
        static void Init();

        //! Init specified AutoGen registry by given name
        static void Init(const char* registryName);

        //! Get component descriptors from all AutoGen registries
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

        //! Get component descriptors from specified AutoGen registries
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors(const char* registryName);

        //! Reflect all AutoGen registries
        static void Reflect(AZ::ReflectContext* context);

        //! Reflect specified AutoGen registry by given name
        static void Reflect(AZ::ReflectContext* context, const char* registryName);

        //! Get all expected autogen registry names
        AZStd::vector<std::string> GetRegistryNames(const char* registryName);

        //! Register autogen registry with its name
        void RegisterRegistry(const char* registryName, IScriptCanvasRegistry* registry);

        //! Unregister autogen function registry by using its name
        void UnregisterRegistry(const char* registryName);

        std::unordered_map<std::string, IScriptCanvasRegistry*> m_registries;
    };
} // namespace ScriptCanvas
