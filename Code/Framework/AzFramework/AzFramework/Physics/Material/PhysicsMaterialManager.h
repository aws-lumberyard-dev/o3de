/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    class MaterialManager
    {
    public:
        AZ_RTTI(Physics::MaterialManager, "{39EF1222-BE2E-461D-B517-0395CF82C156}");

        MaterialManager() = default;
        virtual ~MaterialManager();

        AZStd::shared_ptr<Material2> FindOrCreateMaterial(const MaterialId2& id, const AZ::Data::Asset<MaterialAsset>& materialAsset);

        void DeleteMaterial(const MaterialId2& id);

        void DeleteAllMaterials();

        //! Get default material.
        AZStd::shared_ptr<Material2> GetDefaultMaterial();

        //! Returns a weak pointer to physics material with the given id.
        AZStd::shared_ptr<Material2> GetMaterial(const MaterialId2& id);

    protected:
        virtual AZStd::shared_ptr<Material2> CreateMaterialInternal(const MaterialId2& id, const AZ::Data::Asset<MaterialAsset>& materialAsset) = 0;

        AZStd::shared_ptr<Material2> m_defaultMaterial; // Instantiated and added to m_materials by specialized class.
        AZStd::unordered_map<MaterialId2, AZStd::shared_ptr<Material2>> m_materials;
    };
} // namespace Physics
