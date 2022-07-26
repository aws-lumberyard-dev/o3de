/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverridePublicInterface
        {
        public:
            AZ_RTTI(PrefabOverridePublicInterface, "{19F080A2-BDD7-476F-AA50-C1581401FC81}");

            virtual bool IsOverridePresent(AZ::EntityId entityId) = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
