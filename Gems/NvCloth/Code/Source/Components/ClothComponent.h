/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/SkinnedMesh/SkinnedMeshOverrideBus.h>

#include <Components/ClothConfiguration.h>
#include <Components/ClothComponentMesh/ClothComponentMesh.h>

namespace NvCloth
{
    //! Class for runtime Cloth Component.
    class ClothComponent
        : public AZ::Component
        , public AZ::Render::MeshComponentNotificationBus::Handler
        , public AZ::Render::SkinnedMeshOverrideNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ClothComponent, "{AC9B8FA0-A6DA-4377-8219-25BA7E4A22E9}");

        static void Reflect(AZ::ReflectContext* context);

        ClothComponent() = default;
        explicit ClothComponent(const ClothConfiguration& config);

        AZ_DISABLE_COPY_MOVE(ClothComponent);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        const ClothComponentMesh* GetClothComponentMesh() const { return m_clothComponentMesh.get(); }

    protected:
        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::Render::MeshComponentNotificationBus::Handler overrides ...
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, const AZ::Data::Instance<AZ::RPI::Model>& model) override;
        void OnModelPreDestroy() override;

        // AZ::Render::SkinnedMeshOverrideNotificationBus::Handler
        void OnOverrideSkinning(
            AZStd::intrusive_ptr<const AZ::Render::SkinnedMeshInputBuffers> skinnedMeshInputBuffers,
            AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInstance> skinnedMeshInstance) override;

    private:
        ClothConfiguration m_config;

        AZStd::unique_ptr<ClothComponentMesh> m_clothComponentMesh;
    };

    void OverrideSkinning(
        AZStd::intrusive_ptr<const AZ::Render::SkinnedMeshInputBuffers> skinnedMeshInputBuffers,
        AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInstance> skinnedMeshInstance);
} // namespace NvCloth
