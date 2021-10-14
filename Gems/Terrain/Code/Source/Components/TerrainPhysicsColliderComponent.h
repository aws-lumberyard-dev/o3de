/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Physics/Material.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <TerrainSystem/TerrainSystemBus.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>


namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    struct TerrainPhysicsSurfaceMaterialMapping final
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsSurfaceMaterialMapping, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainPhysicsSurfaceMaterialMapping, "{A88B5289-DFCD-4564-8395-E2177DFE5B18}");
        static void Reflect(AZ::ReflectContext* context);

        SurfaceData::SurfaceTag m_surfaceTag;
        Physics::MaterialId m_materialId;
    };

    class TerrainPhysicsColliderConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainPhysicsColliderConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainPhysicsColliderConfig, "{E9EADB8F-C3A5-4B9C-A62D-2DBC86B4CE59}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<TerrainPhysicsSurfaceMaterialMapping> m_surfaceMaterialMappings;
    };


    class TerrainPhysicsColliderComponent
        : public AZ::Component
        , protected AZ::TransformNotificationBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
        , protected Physics::HeightfieldProviderRequestsBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainPhysicsColliderComponent, "{33C20287-1D37-44D0-96A0-2C3766E23624}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainPhysicsColliderComponent(const TerrainPhysicsColliderConfig& configuration);
        TerrainPhysicsColliderComponent();
        ~TerrainPhysicsColliderComponent() = default;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        uint8_t FindSurfaceTagIndex(const SurfaceData::SurfaceTag tag) const;

        void GetHeightfieldBounds(const AZ::Aabb& bounds, AZ::Vector3& minBounds, AZ::Vector3& maxBounds) const;
        void GetHeightfieldGridSizeInBounds(const AZ::Aabb& bounds, int32_t& numColumns, int32_t& numRows) const;
        void GenerateHeightsInBounds(const AZ::Aabb& bounds, AZStd::vector<float>& heights) const;
        void GenerateHeightsAndMaterialsInBounds(const AZ::Aabb& bounds, AZStd::vector<Physics::HeightMaterialPoint>& heightMaterials) const;

        void NotifyListenersOfHeightfieldDataChange();

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // HeightfieldProviderRequestsBus
        AZ::Vector2 GetHeightfieldGridSpacing() const override;
        void GetHeightfieldGridSize(int32_t& numColumns, int32_t& numRows) const override;
        AZStd::vector<Physics::MaterialId> GetMaterialList() const override;
        AZStd::vector<float> GetHeights() const override;
        AZStd::vector<Physics::HeightMaterialPoint> GetHeightsAndMaterials() const override;
        AZStd::vector<float> UpdateHeights(const AZ::Aabb& dirtyRegion) const override;
        AZStd::vector<Physics::HeightMaterialPoint> UpdateHeightsAndMaterials(const AZ::Aabb& dirtyRegion) const override;
    private:
        TerrainPhysicsColliderConfig m_configuration;
    };
}
