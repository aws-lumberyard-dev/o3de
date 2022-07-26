/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverridePublicHandler : private PrefabOverridePublicInterface
        {
        public:
            PrefabOverridePublicHandler();
            virtual ~PrefabOverridePublicHandler();

            bool IsOverridePresent(AZ::EntityId entityId) override;
        private:
            PrefabOverrideHandler m_prefabOverrideHandler;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
