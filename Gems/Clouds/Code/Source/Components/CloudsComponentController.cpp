/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <Rendering/CloudsFeatureProcessor.h>
#include <Components/CloudsComponentController.h>

namespace Clouds
{
    void CloudsComponentController::Reflect(AZ::ReflectContext* context)
    {
        CloudsComponentConfig::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CloudsComponentController>()
                ->Version(1)
                ->Field("Configuration", &CloudsComponentController::m_config)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<CloudsComponentRequestBus>("CloudsComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ;
        }
    }

    void CloudsComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CloudsService"));
    }

    void CloudsComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CloudsService"));
    }

    void CloudsComponentController::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // add dependencies here
    }

    CloudsComponentController::CloudsComponentController(const CloudsComponentConfig& config)
        : m_config(config)
    {
    }

    void CloudsComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        if (auto scene = AZ::RPI::Scene::GetSceneForEntityId(entityId))
        {
            m_featureProcessor = scene->EnableFeatureProcessor<CloudsFeatureProcessor>();
        }

        if (m_featureProcessor)
        {
            // do any necessary feature processor tasks
        }

        CloudsComponentRequestBus::Handler::BusConnect(m_entityId);
    }

    void CloudsComponentController::Deactivate()
    {
        if (auto scene = AZ::RPI::Scene::GetSceneForEntityId(m_entityId); scene)
        {
            if (scene->GetFeatureProcessor<CloudsFeatureProcessor>())
            {
                scene->DisableFeatureProcessor<CloudsFeatureProcessor>();
            }
        }

        m_featureProcessor = nullptr;

        CloudsComponentRequestBus::Handler::BusDisconnect(m_entityId);
        m_entityId.SetInvalid();
    }

    void CloudsComponentController::SetConfiguration(const CloudsComponentConfig& config)
    {
        m_config = config;
        OnCloudsConfigChanged();
    }

    const CloudsComponentConfig& CloudsComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void CloudsComponentController::OnCloudsConfigChanged()
    {
        // the config changed, handle any necessary tasks here
    }

} // namespace Clouds
