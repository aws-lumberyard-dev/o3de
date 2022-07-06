/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverrideHandler
            : private PrefabOverrideInterface
        {
        public:
            PrefabOverrideHandler();
            virtual ~PrefabOverrideHandler();

            void RegisterOverridePrefix(AZ::Dom::Path path, AZStd::weak_ptr<AZ::Dom::Value> value) override;
            bool IsOverridePresent(AZ::Dom::Path path) override;
            void PrintOverrides() override;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
