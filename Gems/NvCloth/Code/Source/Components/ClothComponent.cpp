/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/Console.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Components/ClothComponent.h>

namespace NvCloth
{
    void ClothComponent::Reflect(AZ::ReflectContext* context)
    {
        ClothConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothComponent, AZ::Component>()
                ->Version(0)
                ->Field("ClothConfiguration", &ClothComponent::m_config)
                ;
        }
    }

    ClothComponent::ClothComponent(const ClothConfiguration& config)
        : m_config(config)
    {
    }
    
    void ClothComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ClothMeshService", 0x6ffcbca5));
    }
    
    void ClothComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MeshService", 0x71d8a455));
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void ClothComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void ClothComponent::Init()
    {
        AZ::Component::Init();

        // The actor component is going to activate before cloth, since the MeshService it provides is a dependent service
        // But there is an event it will send during Activate that needs to be handled to tell the actor to skip skinning
        // The cloth component does not need to be active to handle it, but it needs to be listening, so connect here
        AZ::Render::SkinnedMeshOverrideNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ClothComponent::Activate()
    {
        // Cloth components do not run on dedicated servers.
        if (auto* console = AZ::Interface<AZ::IConsole>::Get())
        {
            bool isDedicated = false;
            if (const auto result = console->GetCvarValue("sv_isDedicated", isDedicated);
                result == AZ::GetValueResult::Success && isDedicated)
            {
                return;
            }
        }

        AZ::Render::SkinnedMeshOverrideNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ClothComponent::Deactivate()
    {
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();
        AZ::Render::SkinnedMeshOverrideNotificationBus::Handler::BusDisconnect();

        m_clothComponentMesh.reset();
    }

    void ClothComponent::OnModelReady(
        const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset,
        [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        if (!asset.IsReady())
        {
            return;
        }

        m_clothComponentMesh = AZStd::make_unique<ClothComponentMesh>(GetEntityId(), m_config);
    }

    void ClothComponent::OnModelPreDestroy()
    {
        m_clothComponentMesh.reset();
    }

    void ClothComponent::OnOverrideSkinning(
        AZStd::intrusive_ptr<const AZ::Render::SkinnedMeshInputBuffers> skinnedMeshInputBuffers,
        AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInstance> skinnedMeshInstance)
    {
        OverrideSkinning(skinnedMeshInputBuffers, skinnedMeshInstance);
    }

    void OverrideSkinning(
        AZStd::intrusive_ptr<const AZ::Render::SkinnedMeshInputBuffers> skinnedMeshInputBuffers,
        AZStd::intrusive_ptr<AZ::Render::SkinnedMeshInstance> skinnedMeshInstance)
    {
        // Iterate through all the meshes
        const auto& modelAsset = skinnedMeshInputBuffers->GetModelAsset();
        for (size_t lodIndex = 0; lodIndex < modelAsset->GetLodAssets().size(); ++lodIndex)
        {
            const auto& lodAsset = modelAsset->GetLodAssets()[lodIndex];
            for (size_t meshIndex = 0; meshIndex < lodAsset->GetMeshes().size(); ++meshIndex)
            {
                // For any mesh with cloth data, disable skinning since
                const bool hasClothData = lodAsset->GetMeshes()[meshIndex].GetSemanticBufferAssetView(AZ::Name("CLOTH_DATA")) != nullptr;
                if (hasClothData)
                {
                    skinnedMeshInstance->DisableSkinning(aznumeric_caster(lodIndex), aznumeric_caster(meshIndex));
                }
            }
        }
    }
} // namespace NvCloth
