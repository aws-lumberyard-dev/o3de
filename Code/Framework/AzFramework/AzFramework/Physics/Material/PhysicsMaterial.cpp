/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Material/PhysicsMaterial.h>

namespace Physics
{
    MaterialId2 Material2::GetId() const
    {
        return m_id;
    }

    AZ::Data::Asset<MaterialAsset> Material2::GetMaterialAsset() const
    {
        return m_materialAsset;
    }
} // namespace Physics
