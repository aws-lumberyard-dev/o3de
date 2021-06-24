/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "GradientSignal_precompiled.h"
#include "EditorImageGradientComponent.h"
#include "EditorImageGradientComponentMode.h"

namespace GradientSignal
{
    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorImageGradientComponent, BaseClassType>()
                ->Version(1)
                ->Field("ComponentMode", &EditorImageGradientComponent::m_componentModeDelegate)
                ->Field("PaintBrush", &EditorImageGradientComponent::m_paintBrush)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorImageGradientComponent>(s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorImageGradientComponent::m_paintBrush, "Paint Brush", "Paint Brush Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::InComponentMode)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_componentModeDelegate,
                        "Component Mode", "Image Gradient Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    bool EditorImageGradientComponent::InComponentMode() const
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    void EditorImageGradientComponent::Activate()
    {
        BaseClassType::Activate();
        m_paintBrush.Activate(AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorImageGradientComponent, EditorImageGradientComponentMode>(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
    }

    void EditorImageGradientComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();
        m_paintBrush.Deactivate();
        BaseClassType::Deactivate();
    }
}
