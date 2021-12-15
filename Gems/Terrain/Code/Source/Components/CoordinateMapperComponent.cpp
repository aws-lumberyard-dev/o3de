/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/CoordinateMapperComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

namespace Terrain
{
    void CoordinateMapperConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CoordinateMapperConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("OriginLatLong", &CoordinateMapperConfig::m_originLatLong)
                ->Field("Scale", &CoordinateMapperConfig::m_scale)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<CoordinateMapperConfig>("Terrain World Component", "Data required for the terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateMapperConfig::m_originLatLong, "Lat / Long", "Latitude and Longitude at the origin")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CoordinateMapperConfig::m_scale, "Scale (m)", "")
                ;
            }
        }
    }

    void CoordinateMapperComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("CoordinateMapperService"));
    }

    void CoordinateMapperComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("CoordinateMapperService"));
    }

    void CoordinateMapperComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void CoordinateMapperComponent::Reflect(AZ::ReflectContext* context)
    {
        CoordinateMapperConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CoordinateMapperComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &CoordinateMapperComponent::m_configuration)
            ;
        }
    }

    CoordinateMapperComponent::CoordinateMapperComponent(const CoordinateMapperConfig& configuration)
        : m_configuration(configuration)
    {
    }

    CoordinateMapperComponent::~CoordinateMapperComponent()
    {
    }

    void CoordinateMapperComponent::Activate()
    {
        CoordinateMapperRequestBus::Handler::BusConnect();
    }

    void CoordinateMapperComponent::Deactivate()
    {
        CoordinateMapperRequestBus::Handler::BusDisconnect();
    }

    bool CoordinateMapperComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const CoordinateMapperConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool CoordinateMapperComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<CoordinateMapperConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void CoordinateMapperComponent::ConvertWorldAabbToLatLong(
        const AZ::Aabb& worldAabb, float& topLatitude, float& leftLongitude, float& bottomLatitude, float& rightLongitude)
    {
        const float degreesToMeters = 111139.0f;
        const float metersToDegrees = 1.0f / degreesToMeters;

        bottomLatitude = (worldAabb.GetMin().GetX() * metersToDegrees / m_configuration.m_scale) + m_configuration.m_originLatLong.GetX();
        leftLongitude = (worldAabb.GetMin().GetY() * metersToDegrees / m_configuration.m_scale) + m_configuration.m_originLatLong.GetY();

        topLatitude = (worldAabb.GetMax().GetX() * metersToDegrees / m_configuration.m_scale) + m_configuration.m_originLatLong.GetX();
        rightLongitude = (worldAabb.GetMax().GetY() * metersToDegrees / m_configuration.m_scale) + m_configuration.m_originLatLong.GetY();
    }


} // namespace Terrain
