/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverrideInterface
        {
        public:
            AZ_RTTI(PrefabFocusInterface, "{CF2B9E08-9235-4E4F-81A5-1A20E0DF453B}");

            virtual bool IsOverridePresent(AZ::Dom::Path path, LinkId linkId) = 0;
        };
    } // namespace Prefab
}


