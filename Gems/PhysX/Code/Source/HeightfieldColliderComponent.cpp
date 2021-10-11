/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Utils.h>

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

    void HeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXTriggerService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void HeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void HeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void HeightfieldColliderComponent::Activate()
    {
        BaseColliderComponent::Activate();
        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::BroadcastResult(samples,
            &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);
    }

    void HeightfieldColliderComponent::Deactivate()
    {
        BaseColliderComponent::Deactivate();
    }

    void HeightfieldColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() != 1)
        {
            AZ_Error(
                "PhysX Heightfield Collider Component", false, "Expected exactly one collider/shape configuration for entity \"%s\".",
                GetEntity()->GetName().c_str());
            return;
        }

        m_shapeConfigList[0].second->m_scale = Utils::GetTransformScale(GetEntityId());
    }

    void HeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void HeightfieldColliderComponent::RefreshHeightfield()
    {
        Physics::HeightfieldShapeConfiguration& configuration =
            static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfigList[0].second);
        configuration = Physics::HeightfieldShapeConfiguration(GetEntityId());

        configuration.SetCachedNativeHeightfield(nullptr);

        int32_t numRows = 0;
        int32_t numColumns = 0;
        Physics::HeightfieldProviderRequestsBus::Broadcast(
            &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);

        configuration.SetNumRows(numRows);
        configuration.SetNumColumns(numColumns);

        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::BroadcastResult(
            samples, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        configuration.SetSamples(samples);
    }



} // namespace PhysX
