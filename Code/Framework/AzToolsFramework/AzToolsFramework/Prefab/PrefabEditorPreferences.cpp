/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>

namespace AzToolsFramework::Prefab
{
    bool IsHotReloadingEnabled()
    {
        bool isHotReloadingEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isHotReloadingEnabled, HotReloadToggleKey);
        }

        return isHotReloadingEnabled;
    }

    bool IsPrefabOverridesUxEnabled()
    {
        bool prefabOverridesUxEnabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(prefabOverridesUxEnabled, EnablePrefabOverridesUxKey);
        }

        return prefabOverridesUxEnabled;
    }

    bool IsInspectorOverrideManagementEnabled()
    {
        bool isInspectorOverrideManagementEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isInspectorOverrideManagementEnabled, InspectorOverrideManagementKey);
        }

        return isInspectorOverrideManagementEnabled;
    }

} // namespace AzToolsFramework::Prefab
