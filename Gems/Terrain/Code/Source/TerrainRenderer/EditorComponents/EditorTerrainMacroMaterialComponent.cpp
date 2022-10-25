/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponent.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponentMode.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

namespace Terrain
{
    void EditorTerrainMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorTerrainMacroMaterialComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(2)
                ->Field("Configuration", &EditorTerrainMacroMaterialComponent::m_configuration)
                ->Field("ComponentMode", &EditorTerrainMacroMaterialComponent::m_componentModeDelegate);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorTerrainMacroMaterialComponent>(
                        EditorTerrainMacroMaterialComponent::s_componentName, EditorTerrainMacroMaterialComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, EditorTerrainMacroMaterialComponent::s_icon)
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, EditorTerrainMacroMaterialComponent::s_viewportIcon)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, EditorTerrainMacroMaterialComponent::s_helpUrl)
                        ->Attribute(AZ::Edit::Attributes::Category, EditorTerrainMacroMaterialComponent::s_categoryName)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Configuration for the Image Gradient control itself
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorTerrainMacroMaterialComponent::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorTerrainMacroMaterialComponent::ConfigurationChanged)

                    // Paint controls for editing the image
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorTerrainMacroMaterialComponent::m_componentModeDelegate,
                        "Paint Image",
                        "Paint into an image asset")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    // The following methods pass through to the runtime component so that the Editor component shares the same requirements.

    void EditorTerrainMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetRequiredServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetIncompatibleServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        TerrainMacroMaterialComponent::GetProvidedServices(services);
    }

    void EditorTerrainMacroMaterialComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        //TerrainMacroMaterialComponent::GetDependentServices(services);
    }

    void EditorTerrainMacroMaterialComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        // When building the game entity, use the copy of the runtime configuration on the Editor component to create
        // a new runtime component that's configured correctly.
        gameEntity->AddComponent(aznew TerrainMacroMaterialComponent(m_configuration));
    }

    void EditorTerrainMacroMaterialComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();

        // Initialize the copy of the runtime component.
        m_runtimeComponentActive = false;
        m_component.ReadInConfig(&m_configuration);
        m_component.Init();
    }

    void EditorTerrainMacroMaterialComponent::Activate()
    {
        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();

            // Use the visibility bus to control whether or not the runtime gradient is active and processing in the Editor.
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                m_visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

            // Synchronize the runtime component with the Editor component.
            m_component.ReadInConfig(&m_configuration);
            m_component.SetEntity(GetEntity());

            if (m_visible)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        auto entityComponentIdPair = AZ::EntityComponentIdPair(GetEntityId(), GetId());
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorTerrainMacroMaterialComponent, EditorTerrainMacroMaterialComponentMode>(
            entityComponentIdPair, nullptr);
    }

    void EditorTerrainMacroMaterialComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        // This block of code is aligned with EditorWrappedComponentBase
        {
            AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();

            m_runtimeComponentActive = false;
            m_component.Deactivate();
            // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
            m_component.SetEntity(nullptr);
        }
    }

    void EditorTerrainMacroMaterialComponent::OnEntityVisibilityChanged(bool visibility)
    {
        if (m_visible != visibility)
        {
            m_visible = visibility;
            ConfigurationChanged();
        }
    }

    void EditorTerrainMacroMaterialComponent::OnCompositionChanged()
    {
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();
    }

    AZ::u32 EditorTerrainMacroMaterialComponent::ConfigurationChanged()
    {
        // This block of code aligns with EditorWrappedComponentBase
        {
            if (m_runtimeComponentActive)
            {
                m_runtimeComponentActive = false;
                m_component.Deactivate();
            }

            m_component.ReadInConfig(&m_configuration);

            if (m_visible && !m_runtimeComponentActive)
            {
                m_component.Activate();
                m_runtimeComponentActive = true;
            }
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return AZ::Edit::PropertyRefreshLevels::None;
    }

} // namespace Terrain
