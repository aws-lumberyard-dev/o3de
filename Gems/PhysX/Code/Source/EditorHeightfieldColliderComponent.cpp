/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorHeightfieldColliderComponent.h>

namespace PhysX
{
    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        context;
    }
    

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

} // namespace PhysX
