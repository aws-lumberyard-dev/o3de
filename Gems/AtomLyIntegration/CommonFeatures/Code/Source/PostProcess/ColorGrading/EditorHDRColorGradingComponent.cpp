/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/ColorGrading/EditorHDRColorGradingComponent.h>

namespace AZ
{
    namespace Render
    {
        void EditorHDRColorGradingComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorHDRColorGradingComponent, BaseClass>()->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorHDRColorGradingComponent>(
                        "HDR Color Grading", "Tune and apply color grading in HDR.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::Category, "Atom")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg") // [GFX TODO ATOM-2672][PostFX] need to create icons for PostProcessing.
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                        ->Attribute(Edit::Attributes::HelpPageURL, "https://") // [TODO ATOM-2672][PostFX] need to create page for PostProcessing.
                        ;

                    editContext->Class<HDRColorGradingComponentController>(
                        "HDRColorGradingComponentControl", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentController::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<HDRColorGradingComponentConfig>("LookModificationComponentConfig", "")
                        ->DataElement(Edit::UIHandlers::CheckBox, &HDRColorGradingComponentConfig::m_enabled,
                            "Enable look modification",
                            "Enable look modification.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &HDRColorGradingComponentConfig::m_colorGradingPostSaturation, "Post Saturation", "Post Saturation Value");
                        // TODO add all options
                }
            }
        }

        EditorHDRColorGradingComponent::EditorHDRColorGradingComponent(const HDRColorGradingComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorHDRColorGradingComponent::OnConfigurationChanged()
        {
            m_controller.OnConfigChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    } // namespace Render
} // namespace AZ
