/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorForceRegionComponent, EditorForceRegionComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace PhysX
