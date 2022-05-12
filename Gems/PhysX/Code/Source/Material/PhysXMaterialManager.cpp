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
    AZStd::shared_ptr<Physics::Material2> MaterialManager::CreateDefaultMaterialInternal()
    {
        // Create default material asset
        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset =
            AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(
                AZ::Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);

        // Create default material with random id
        return CreateMaterialInternal(
            Physics::MaterialId2::CreateRandom(), defaultMaterialAsset);
    }

    AZStd::shared_ptr<Physics::Material2> MaterialManager::CreateMaterialInternal(
        const Physics::MaterialId2& id,
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::shared_ptr<Physics::Material2>(
            aznew Material2(id, materialAsset));
    }
} // namespace PhysX
