/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Clouds/CloudsComponentBus.h>
#include <Components/CloudsComponentConfig.h>

namespace Clouds
{
    class CloudsFeatureProcessor;

    //! This is the controller class for both EditorComponent and in game Component.
    class CloudsComponentController final
        : public CloudsComponentRequestBus::Handler
    {
    public:
        friend class CloudsEditorComponent;

        AZ_TYPE_INFO(Clouds::CloudsComponentController, "{A86C20D6-F4E8-4655-A72A-27EDAE884E46}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        CloudsComponentController() = default;
        CloudsComponentController(const CloudsComponentConfig& config);

        void Activate(AZ::EntityId entityId);
        void Deactivate();

        void SetConfiguration(const CloudsComponentConfig& config);
        const CloudsComponentConfig& GetConfiguration() const;

    private:
        AZ_DISABLE_COPY(CloudsComponentController);

        void OnCloudsConfigChanged();

        //! CloudsComponentRequestBus

        CloudsFeatureProcessor* m_featureProcessor = nullptr;
        CloudsComponentConfig m_config;
        AZ::EntityId m_entityId;
    };

} // namespace Clouds
