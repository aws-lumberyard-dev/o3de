/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Components/CloudsEditorComponent.h>

namespace Clouds
{
    void CloudsEditorComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CloudsEditorComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CloudsEditorComponent>(
                    "Clouds", "Controls Clouds Properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
                
                editContext->Class<CloudsComponentController>(
                    "CloudsComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CloudsComponentController::m_config, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;

                editContext->Class<CloudsComponentConfig>(
                    "CloudsComponentConfig", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &CloudsComponentConfig::m_enabled, "Enabled",
                        "Enabled")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<CloudsEditorComponent>()->RequestBus("CloudsRequestsBus");

            behaviorContext->ConstantProperty("CloudsEditorComponentTypeId", BehaviorConstant(AZ::Uuid(Clouds::CloudsEditorComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    CloudsEditorComponent::CloudsEditorComponent(const CloudsComponentConfig& config)
        : BaseClass(config)
    {
    }

    void CloudsEditorComponent::Activate()
    {
        BaseClass::Activate();
    }

    void CloudsEditorComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    AZ::u32 CloudsEditorComponent::OnConfigurationChanged()
    {
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace Clouds
