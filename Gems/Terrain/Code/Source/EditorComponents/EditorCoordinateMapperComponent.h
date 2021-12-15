/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Components/CoordinateMapperComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorCoordinateMapperComponent
        : public LmbrCentral::EditorWrappedComponentBase<CoordinateMapperComponent, CoordinateMapperConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<CoordinateMapperComponent, CoordinateMapperConfig>;
        AZ_EDITOR_COMPONENT(EditorCoordinateMapperComponent, "{494872D4-FDE2-4E3E-90C8-0DE9C4AD898F}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        AZ::u32 ConfigurationChanged() override;

    protected:
        using BaseClassType::m_configuration;
        using BaseClassType::m_component;
        using BaseClassType::m_visible;

    private:
    };
}
