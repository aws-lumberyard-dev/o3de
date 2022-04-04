/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <LuaDebuggerEditorBus.h>
#include <LuaDebuggerSystemComponent.h>

#include <AzCore/Script/ScriptAsset.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace LuaDebugger
{
    /// System component for LuaDebugger editor
    class LuaDebuggerEditorSystemComponent
        : public LuaDebuggerSystemComponent
        , private LuaDebuggerEditorRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    {
        using BaseSystemComponent = LuaDebuggerSystemComponent;
    public:
        AZ_COMPONENT(LuaDebuggerEditorSystemComponent, "{a3f36d4d-538d-4385-967b-1aa63295e87e}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        LuaDebuggerEditorSystemComponent();
        ~LuaDebuggerEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // EditorEntityContextNotificationBus overrides ...
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;

        // LuaDebuggerRequestBus overrides ...
        void ConnectToDebugger() override;
        void ConnectToDebuggerWithAddress(const char* host, AZ::u16 port) override;
        void DisconnectFromDebugger() override;
        bool IsConnectedToDebugger() const override;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // --- Private Member Functions ---
        bool Startup();
        void Shutdown();

        // --- Private Member Data ---
        bool m_startedUp = false;
        bool m_connected = false;
        AZ::Data::Asset<AZ::ScriptAsset> m_debugScriptAsset;
    };
} // namespace LuaDebugger
