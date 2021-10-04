/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BaseColliderComponent.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace PhysX
{
    //! Component that provides Heightfield Collider.
    class HeightfieldColliderComponent
        : public BaseColliderComponent
        , protected Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        using Configuration = Physics::HeightfieldShapeConfiguration;
        AZ_COMPONENT(HeightfieldColliderComponent, "{9A42672C-281A-4CE8-BFDD-EAA1E0FCED76}", BaseColliderComponent);
        static void Reflect(AZ::ReflectContext* context);

        HeightfieldColliderComponent() = default;

        void Activate() override;
        void Deactivate() override;

        // BaseColliderComponent
        void UpdateScaleForShapeConfigs() override;
    };
} // namespace PhysX
