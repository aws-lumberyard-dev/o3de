/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>

namespace AzToolsFramework::Prefab
{
    class PrefabComponentEditPatcher
    {
    public:
        PrefabComponentEditPatcher() = default;
        bool CreateAndApplyComponentEditPatch(
            AZStd::string_view relativePathFromOwningPrefab,
            const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo,
            AZ::EntityId entityId);

        bool CreateAndApplyComponentOverridePatch(
            AZ::Dom::Path relativePathFromOwningPrefab,
            const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo,
            AZ::EntityId entityId);
    };
}
