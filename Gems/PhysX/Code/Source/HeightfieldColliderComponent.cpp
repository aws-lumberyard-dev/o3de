/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/HeightfieldColliderComponent.h>

namespace PhysX
{
    void HeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ;
        }
    }
} // namespace PhysX
