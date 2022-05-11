/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Asset/AssetManager.h>

#include <PhysX/Material/PhysXMaterial.h>
#include <Source/Material/PhysXMaterialManager.h>

namespace PhysX
{
    MaterialManager::MaterialManager()
    {
        // Create default material
        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset =
            AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(
                AZ::Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);

        m_defaultMaterial = CreateMaterialInternal(
            Physics::MaterialId2::CreateRandom(), defaultMaterialAsset);

        // Add default material to the list
        m_materials.emplace(m_defaultMaterial->GetId(), m_defaultMaterial);
    }

    AZStd::shared_ptr<Physics::Material2> MaterialManager::CreateMaterialInternal(
        const Physics::MaterialId2& id,
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::shared_ptr<Physics::Material2>(
            aznew Material2(id, materialAsset));
    }
} // namespace PhysX
