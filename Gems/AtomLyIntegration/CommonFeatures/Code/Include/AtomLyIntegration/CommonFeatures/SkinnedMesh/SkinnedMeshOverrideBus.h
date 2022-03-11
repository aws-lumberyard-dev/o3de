/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace Render
    {
        /**
         * SkinnedMeshOverrideBus provides an interface for components to disable skinning
         */
        class SkinnedMeshOverrideNotifications
            : public ComponentBus
        {
        public:
            virtual void OnOverrideSkinning(AZStd::intrusive_ptr<const SkinnedMeshInputBuffers> skinnedMeshInputBuffers, AZStd::intrusive_ptr<SkinnedMeshInstance> skinnedMeshInstance) = 0;
        };
        using SkinnedMeshOverrideNotificationBus = EBus<SkinnedMeshOverrideNotifications>;
    } // namespace Render
} // namespace AZ
