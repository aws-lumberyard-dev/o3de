/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Script/ScriptAsset.h>

#include <LuaDebugger/LuaDebuggerBus.h>

namespace LuaDebugger
{
#if defined(AZ_DEBUG_BUILD)

    class LuaDebuggerSystemComponent
        : public AZ::Component
        , public LuaDebuggerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LuaDebuggerSystemComponent, "{EFA1DB09-93AD-4282-BDD4-96F79908CA50}");

        // --- Construction/Initialization ---
        LuaDebuggerSystemComponent() = default;

        static void Reflect(AZ::ReflectContext* reflectContext);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // --- AZ::Component Overrides ---
        void Activate() override;
        void Deactivate() override;

        // --- LuaDebuggerRequestBus Overrides ---
        void ConnectToDebugger() override;
        void ConnectToDebuggerWithAddress(const char* host, AZ::u16 port) override;
        void DisconnectFromDebugger() override;
        bool IsConnectedToDebugger() const override;
        
    private:
        // --- Private Member Functions ---
        bool Startup();
        void Shutdown();

        // --- Private Member Data ---
        bool m_startedUp = false;
        bool m_connected = false;
        AZ::Data::Asset<AZ::ScriptAsset> m_debugScriptAsset;
    };

#endif // AZ_DEBUG_BUILD)
}
