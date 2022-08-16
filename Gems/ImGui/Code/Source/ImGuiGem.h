/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <IGem.h>
#include "ImGuiManager.h"
#include <AzCore/Module/Module.h>
//#include "Private/ImGuiManagerImpl.h"
#ifdef IMGUI_ENABLED
#include "LYCommonMenu/ImGuiLYCommonMenu.h"
#endif //IMGUI_ENABLED

namespace ImGui
{
    /*!
    * The ImGui::Module class coordinates with the application
    * to reflect classes and create system components.
    */
    class ImGuiModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ImGuiModule, "{C8BD76C6-27B0-4A2D-8A95-7F3576633C48}", AZ::Module);

        ImGuiModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:
        #ifdef IMGUI_ENABLED
        ImGuiLYCommonMenu lyCommonMenu;
        ImGuiManager manager;
        #endif
    };
}
