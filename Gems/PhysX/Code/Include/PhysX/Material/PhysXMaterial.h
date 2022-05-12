/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <PxPhysicsAPI.h>

namespace Physics
{
    class MaterialSlots;
};

namespace PhysX
{
    //! Runtime PhysX material, created from a MaterialConfiguration.
    // TODO: Material2 is temporary until old Material class is removed.
    class Material2
        : public Physics::Material2
        , public AZStd::enable_shared_from_this<Material2>
        , protected AZ::Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Material2, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysX::Material2, "{57A9681F-4025-4D66-891B-80CBC78BDEB9}", Physics::Material2);

        static AZStd::shared_ptr<Material2> FindOrCreateMaterial(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);
        static AZStd::vector<AZStd::shared_ptr<Material2>> FindOrCreateMaterials(const Physics::MaterialSlots& materialSlots);
        static AZStd::shared_ptr<Material2> CreateMaterialWithRandomId(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        ~Material2() override;

        // Physics::Material2 overrides ...
        float GetDynamicFriction() const override;
        void SetDynamicFriction(float dynamicFriction) override;
        float GetStaticFriction() const override;
        void SetStaticFriction(float staticFriction) override;
        float GetRestitution() const override;
        void SetRestitution(float restitution) override;
        Physics::CombineMode GetFrictionCombineMode() const override;
        void SetFrictionCombineMode(Physics::CombineMode mode) override;
        Physics::CombineMode GetRestitutionCombineMode() const override;
        void SetRestitutionCombineMode(Physics::CombineMode mode) override;
        float GetDensity() const override;
        void SetDensity(float density) override;
        const AZ::Color& GetDebugColor() const override;
        void SetDebugColor(const AZ::Color& debugColor) override;

        const physx::PxMaterial* GetPxMaterial() const;

    protected:
        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        friend class MaterialManager;

        Material2(
            const Physics::MaterialId2& id,
            const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        float m_density = 1000.0f;
        AZ::Color m_debugColor = AZ::Colors::White;
    };
} // namespace PhysX
