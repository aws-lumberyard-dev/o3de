/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LuaDebuggerEditorSystemComponent.h>
#include <LuaDebuggerScriptSocket.h>
#include <LuaDebuggerEditorBus.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LuaDebugger
{
    class LuaDebuggerToLuaRequests : public AZ::EBusTraits
    {
    public:
        virtual void ConnectSession([[maybe_unused]] const char* host, [[maybe_unused]] AZ::u16 port)
        {
        }
        virtual void DisconnectSession()
        {
        }
        virtual void Shutdown()
        {
        }
    };

    using LuaDebuggerToLuaRequestBus = AZ::EBus<LuaDebuggerToLuaRequests>;

    class LuaDebuggerBehaviorDebugMessagesHandler
        : public LuaDebuggerToLuaRequestBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            LuaDebuggerBehaviorDebugMessagesHandler,
            "{FCE8BDDB-55B4-4185-8848-99699D8E8920}",
            AZ::SystemAllocator,
            ConnectSession,
            DisconnectSession,
            Shutdown);

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

    void LuaDebuggerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LuaDebuggerEditorSystemComponent, LuaDebuggerSystemComponent>()->Version(0);

            if (AZ::EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<LuaDebuggerSystemComponent>("LuaDebugger", "Support for debugging Lua using external IDEs")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LuaDebuggerToLuaRequestBus>("LuaDebuggerToLuaRequestBus")
                ->Handler<LuaDebuggerBehaviorDebugMessagesHandler>();
        }
    }

    LuaDebuggerEditorSystemComponent::LuaDebuggerEditorSystemComponent() = default;

    LuaDebuggerEditorSystemComponent::~LuaDebuggerEditorSystemComponent() = default;

    void LuaDebuggerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("LuaDebuggerEditorService"));
    }

    void LuaDebuggerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("LuaDebuggerEditorService"));
    }

    void LuaDebuggerEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void LuaDebuggerEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);

        dependent.push_back(AZ_CRC("ScriptService", 0x787235ab));
    }

    void LuaDebuggerEditorSystemComponent::Activate()
    {
        LuaDebuggerSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

        LuaDebuggerEditorRequestBus::Handler::BusConnect();
    }

    void LuaDebuggerEditorSystemComponent::Deactivate()
    {
        LuaDebuggerEditorRequestBus::Handler::BusDisconnect();
        DisconnectFromDebugger();
        Shutdown();

        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        LuaDebuggerSystemComponent::Deactivate();
    }

    void LuaDebuggerEditorSystemComponent::OnStartPlayInEditorBegin()
    {
        ConnectToDebugger();
    }

    void LuaDebuggerEditorSystemComponent::OnStopPlayInEditor()
    {
        DisconnectFromDebugger();
    }

    void LuaDebuggerEditorSystemComponent::ConnectToDebugger()
    {
        ConnectToDebuggerWithAddress("127.0.0.1", 8818);
    }

    void LuaDebuggerEditorSystemComponent::ConnectToDebuggerWithAddress([[maybe_unused]] const char* host, [[maybe_unused]] AZ::u16 port)
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

    void LuaDebuggerEditorSystemComponent::DisconnectFromDebugger()
    {
        if (m_connected)
        {
            LuaDebuggerToLuaRequestBus::Broadcast(&LuaDebuggerToLuaRequestBus::Events::DisconnectSession);
            m_connected = false;
        }
    }

    bool LuaDebuggerEditorSystemComponent::IsConnectedToDebugger() const
    {
        return m_connected;
    }

    bool LuaDebuggerEditorSystemComponent::Startup()
    {
        if (!m_debugScriptAsset)
        {
            AZ::Data::AssetId debugScriptAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                debugScriptAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, "Scripts/Debugger/LuaPanda.lua",
                azrtti_typeid<AZ::ScriptAsset>(), true);
            if (debugScriptAssetId.IsValid())
            {
                m_debugScriptAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(
                    debugScriptAssetId, AZ::Data::AssetLoadBehaviorNamespace::AssetLoadBehavior::PreLoad);
            }
        }

        if (!m_startedUp && m_debugScriptAsset)
        {
            AZ::ScriptContext* scriptContext = nullptr;
            AZ::ScriptSystemRequestBus::BroadcastResult(
                scriptContext, &AZ::ScriptSystemRequestBus::Events::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            if (scriptContext)
            {
                // Any use of this * must * not * go * into * the * github * codebase *
                //scriptContext->Execute("local az_searchers = package.searchers; package.searchers = _G.old_searchers; local socket = require(\"socket.core\"); package.searchers = az_searchers; if socket == nil then print(\"SOCKET NIL\") end");


                // pdebug_init(scriptContext->NativeContext());
                //scriptContext->Execute("local socket = package.loadlib(\"socket\", \"luaopen_socket_core\"); if socket == nil then Debug.Log(\"SOCKET NIL\") end");
                scriptContext->Execute("require(\"scripts.debugger.LuaPanda\").start(\"127.0.0.1\", 8818);");
            }

            m_startedUp = true;
        }

        return m_startedUp;
    }

    void LuaDebuggerEditorSystemComponent::Shutdown()
    {
        if (m_startedUp)
        {
            LuaDebuggerToLuaRequestBus::Broadcast(&LuaDebuggerToLuaRequestBus::Events::Shutdown);
            m_startedUp = false;
        }
    }

} // namespace LuaDebugger
