/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma optimize("", off)
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapterInterface.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabPropertyEditorNodes.h>
#include <QtCore/QTimer>

namespace AZ::DocumentPropertyEditor
{
    ComponentAdapter::ComponentAdapter()
    {
        m_propertyChangeHandler = ReflectionAdapter::PropertyChangeEvent::Handler(
            [this](const ReflectionAdapter::PropertyChangeInfo& changeInfo)
            {
                this->GeneratePropertyEditPatch(changeInfo);
            });
        ConnectPropertyChangeHandler(m_propertyChangeHandler);
    }

    ComponentAdapter::ComponentAdapter(AZ::Component* componentInstace)
    {
        m_propertyChangeHandler = ReflectionAdapter::PropertyChangeEvent::Handler(
            [this](const ReflectionAdapter::PropertyChangeInfo& changeInfo)
            {
                this->GeneratePropertyEditPatch(changeInfo);
            });
        ConnectPropertyChangeHandler(m_propertyChangeHandler);
        SetComponent(componentInstace);
    }

    ComponentAdapter::~ComponentAdapter()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_componentInstance->GetEntityId());
    }

    void ComponentAdapter::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (m_componentInstance->GetId() == componentId)
        {
            m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        m_componentInstance = componentInstance;
        m_entityId = m_componentInstance->GetEntityId();
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_entityId);
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();

        // Set the component alias before calling SetValue(). Otherwise, an empty alias will be sent to the PrefabAdapter.
        m_componentAlias = componentInstance->GetSerializedIdentifier();
        auto owningInstance = AZ::Interface<AzToolsFramework::Prefab::InstanceEntityMapperInterface>::Get()->FindOwningInstance(
            componentInstance->GetEntityId());
        AZ_Assert(owningInstance.has_value(), "Entity owning the component doesn't have an owning prefab instance.");
        auto entityAlias = owningInstance->get().GetEntityAlias(componentInstance->GetEntityId());
        AZ_Assert(entityAlias.has_value(), "Owning entity of component doesn't have a valid entity alias in the owning prefab.");
        m_entityAlias = entityAlias->get();


        AZ::Uuid instanceTypeId = azrtti_typeid(m_componentInstance);
        SetValue(m_componentInstance, instanceTypeId);
    }

    void ComponentAdapter::DoRefresh()
    {
        if (m_queuedRefreshLevel == AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None)
        {
            return;
        }
        m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
        NotifyResetDocument();
    }

    /*
    Dom::Value ComponentAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&]([[maybe_unused]] const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            switch (changeType)
            {
            case Nodes::ValueChangeType::InProgressEdit:
                if (m_componentInstance)
                {
                    const AZ::EntityId& entityId = m_componentInstance->GetEntityId();
                    if (entityId.IsValid())
                    {
                        if (m_currentUndoNode)
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode,
                                &AzToolsFramework::ToolsApplicationRequests::ResumeUndoBatch,
                                m_currentUndoNode,
                                "Modify Entity Property");
                        }
                        else
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
                        }

                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, entityId);
                    }
                }
                break;
            case Nodes::ValueChangeType::FinishedEdit:
                if (m_currentUndoNode)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                    m_currentUndoNode = nullptr;
                }
                break;
            }
        };

        Dom::Value returnValue = message.Match(Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged);

        ReflectionAdapter::HandleMessage(message);

        return returnValue;
    }*/

    void ComponentAdapter::CreateLabel(AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath)
    {
        auto* prefabAdapterInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabAdapterInterface>::Get();
        if (prefabAdapterInterface)
        {
            AZ::Dom::Path relativePathFromEntity;
            if (!serializedPath.empty())
            {
                relativePathFromEntity /= AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName;
                relativePathFromEntity /= m_componentAlias;
                relativePathFromEntity /= AZ::Dom::Path(serializedPath);
            }
            prefabAdapterInterface->AddPropertyLabelNode(adapterBuilder, labelText, relativePathFromEntity, m_entityId);
        }
    }

    void ComponentAdapter::GeneratePropertyEditPatch(const ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo)
    {
        if (propertyChangeInfo.changeType == Nodes::ValueChangeType::FinishedEdit)
        {
            auto* prefabAdapterInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabAdapterInterface>::Get();
            if (prefabAdapterInterface != nullptr)
            {
                //AZ::Dom::Value domValue = GetContents();
                AZ::Dom::Path serializedPath = propertyChangeInfo.path / AZ::Reflection::DescriptorAttributes::SerializedPath;

                AZ::Dom::Path relativePathFromOwningPrefab(AzToolsFramework::Prefab::PrefabDomUtils::EntitiesName);
                relativePathFromOwningPrefab /= m_entityAlias;
                relativePathFromOwningPrefab /= AzToolsFramework::Prefab::PrefabDomUtils::ComponentsName;
                relativePathFromOwningPrefab /= m_componentAlias;
                relativePathFromOwningPrefab /= AZ::Dom::Path(GetContents()[serializedPath].GetString());

                prefabAdapterInterface->GeneratePropertyEditPatch(propertyChangeInfo, m_entityId, relativePathFromOwningPrefab);

                AZ::Dom::Path propertyEditorPath = propertyChangeInfo.path;
                propertyEditorPath.Pop();
                AZ::Dom::Value& propertyRowValue = GetContents()[propertyEditorPath];
                if (propertyRowValue[0].IsNode())
                {
                    propertyRowValue[0][AzToolsFramework::Prefab::PrefabPropertyEditorNodes::PrefabOverrideLabel::IsOverridden.GetName()] =
                        true;
                    AZ::Dom::Value domValue1 = GetContents();
                    AZ_Warning("Prefab", domValue1.IsNull() == false, "domvalue is null");
                }
            }
        }
    } 
} // namespace AZ::DocumentPropertyEditor
#pragma optimize("", on)
