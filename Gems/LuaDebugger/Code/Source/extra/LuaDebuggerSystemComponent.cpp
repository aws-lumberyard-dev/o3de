/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LuaDebugger/LuaDebuggerScriptSocket.h>
#include <LuaDebugger/LuaDebuggerSystemComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LuaDebugger
{
#if defined(AZ_DEBUG_BUILD)

    class LuaDebuggerToLuaRequests
        : public AZ::EBusTraits
    {
    public:

        virtual void ConnectSession([[maybe_unused]] const char* host, [[maybe_unused]] AZ::u16 port) {}
        virtual void DisconnectSession() {}
        virtual void Shutdown() {}
    };

    using LuaDebuggerToLuaRequestBus = AZ::EBus<LuaDebuggerToLuaRequests>;

    class LuaDebuggerBehaviorDebugMessagesHandler
        : public LuaDebuggerToLuaRequestBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(LuaDebuggerBehaviorDebugMessagesHandler, "{FCE8BDDB-55B4-4185-8848-99699D8E8920}", AZ::SystemAllocator, ConnectSession, DisconnectSession, Shutdown);

        void ConnectSession(const char* host, AZ::u16 port) override
        {
            Call(FN_ConnectSession, host, port);
        }

        void DisconnectSession() override
        {
            Call(FN_DisconnectSession);
        }

        void Shutdown() override
        {
            Call(FN_Shutdown);
        }
    };

    void LuaDebuggerSystemComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        LuaDebuggerScriptSocket::Reflect(reflectContext);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serialize->Class<LuaDebuggerSystemComponent, AZ::Component>()->Version(0);
                //->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LuaDebuggerSystemComponent>("LuaDebugger", "Support for debugging Lua using external IDEs")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->EBus<LuaDebuggerToLuaRequestBus>("LuaDebuggerToLuaRequestBus")
                ->Handler<LuaDebuggerBehaviorDebugMessagesHandler>()
                ;
        }
    }

    void LuaDebuggerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LuaDebuggerService"));

    }
    void LuaDebuggerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LuaDebuggerService"));
    }

    void LuaDebuggerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("ScriptService", 0x787235ab));
    }

    void LuaDebuggerSystemComponent::Activate()
    {
        LuaDebuggerRequestBus::Handler::BusConnect();
    }

    void LuaDebuggerSystemComponent::Deactivate()
    {
        LuaDebuggerRequestBus::Handler::BusDisconnect();
        DisconnectFromDebugger();
        Shutdown();
    }

    void LuaDebuggerSystemComponent::ConnectToDebugger()
    {
        ConnectToDebuggerWithAddress("127.0.0.1", 8818);
    }

    void LuaDebuggerSystemComponent::ConnectToDebuggerWithAddress([[maybe_unused]] const char* host, [[maybe_unused]] AZ::u16 port)
    {
        if (!Startup())
        {
            return;
        }

        if (!m_connected)
        {
            LuaDebuggerToLuaRequestBus::Broadcast(&LuaDebuggerToLuaRequestBus::Events::ConnectSession, host, port);
            m_connected = true;
        }
    }

    void LuaDebuggerSystemComponent::DisconnectFromDebugger()
    {
        if (m_connected)
        {
            LuaDebuggerToLuaRequestBus::Broadcast(&LuaDebuggerToLuaRequestBus::Events::DisconnectSession);
            m_connected = false;
        }
    }

    bool LuaDebuggerSystemComponent::IsConnectedToDebugger() const
    {
        return m_connected;
    }
    
    bool LuaDebuggerSystemComponent::Startup()
    {
        if (!m_debugScriptAsset)
        {
            AZ::Data::AssetId debugScriptAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                debugScriptAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, "Scripts/Debugger/LuaPanda.lua", azrtti_typeid<AZ::ScriptAsset>(), true);
            if (debugScriptAssetId.IsValid())
            {
                m_debugScriptAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(
                    debugScriptAssetId, AZ::Data::AssetLoadBehaviorNamespace::AssetLoadBehavior::PreLoad);
            }
        }

        if (!m_startedUp && m_debugScriptAsset)
        {
            AZ::ScriptContext* scriptContext = nullptr;
            AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequestBus::Events::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            if (scriptContext)
            {
                //pdebug_init(scriptContext->NativeContext());
                //scriptContext->Execute("LUA_PATH=Engine/Scripts/luasocketBin/win/x64/?.lua;?.lua");
                scriptContext->Execute("LUA_CPATH=Engine/Scripts/luasocketBin/win/x64/?.dll");
                scriptContext->Execute("require(\"socket.core\");");
                scriptContext->Execute("require(\"scripts.debugger.LuaPanda\").start(\"127.0.0.1\", 8818);");
            }

            m_startedUp = true;
        }

        return m_startedUp;
    }

    void LuaDebuggerSystemComponent::Shutdown()
    {
        if (m_startedUp)
        {
            LuaDebuggerToLuaRequestBus::Broadcast(&LuaDebuggerToLuaRequestBus::Events::Shutdown);
            m_startedUp = false;
        }
    }
#endif // AZ_DEBUG_BUILD
}

