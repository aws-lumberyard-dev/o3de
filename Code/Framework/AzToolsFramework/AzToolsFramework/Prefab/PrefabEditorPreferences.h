/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework::Prefab
{
    static constexpr AZStd::string_view EnablePrefabOverridesUxKey = "/O3DE/Preferences/Prefabs/EnableOverridesUx";
    static constexpr AZStd::string_view InspectorOverrideManagementKey = "/O3DE/Preferences/Prefabs/EnableInspectorOverrideManagement";
    static constexpr AZStd::string_view HotReloadToggleKey = "/O3DE/Preferences/Prefabs/EnableHotReloading";

    bool IsHotReloadingEnabled();
    bool IsPrefabOverridesUxEnabled();
    bool IsInspectorOverrideManagementEnabled();

} // namespace AzToolsFramework::Prefab
