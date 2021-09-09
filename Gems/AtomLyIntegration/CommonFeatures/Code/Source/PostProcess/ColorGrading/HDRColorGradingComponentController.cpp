/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/ColorGrading/HDRColorGradingComponentController.h>

namespace AZ
{
    namespace Render
    {
        void HDRColorGradingComponentController::Reflect(ReflectContext* context)
        {
            HDRColorGradingComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<HDRColorGradingComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &HDRColorGradingComponentController::m_configuration);
            }

            // TODO: Add behavior context for ebus
        }

        void HDRColorGradingComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("HDRColorGradingService"));
        }

        void HDRColorGradingComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("HDRColorGradingService"));
        }

        void HDRColorGradingComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        HDRColorGradingComponentController::HDRColorGradingComponentController(const HDRColorGradingComponentConfig& config)
            : m_configuration(config)
        {
        }

        void HDRColorGradingComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            PostProcessFeatureProcessorInterface* fp =
                RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (fp)
            {
                m_postProcessInterface = fp->GetOrCreateSettingsInterface(m_entityId);
                if (m_postProcessInterface)
                {
                    m_settingsInterface = m_postProcessInterface->GetOrCreateHDRColorGradingSettingsInterface();
                    OnConfigChanged();
                }
            }
            //HDRColorGradingRequestBus::Handler::BusConnect(m_entityId);
        }

        void HDRColorGradingComponentController::Deactivate()
        {
            //HDRColorGradingRequestBus::Handler::BusDisconnect(m_entityId);

            if (m_postProcessInterface)
            {
                m_postProcessInterface->RemoveHDRColorGradingSettingsInterface();
            }

            m_postProcessInterface = nullptr;
            m_settingsInterface = nullptr;
            m_entityId.SetInvalid();
        }

        void HDRColorGradingComponentController::SetConfiguration(const HDRColorGradingComponentConfig& config)
        {
            m_configuration = config;
            OnConfigChanged();
        }

        const HDRColorGradingComponentConfig& HDRColorGradingComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void HDRColorGradingComponentController::OnConfigChanged()
        {
            if (m_settingsInterface)
            {
                m_configuration.CopySettingsTo(m_settingsInterface);
                m_settingsInterface->OnConfigChanged();
            }
        }
    } // namespace Render
} // namespace AZ
