// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <OceanSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace Ocean
{
    void OceanSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OceanSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<OceanSystemComponent>("Ocean", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void OceanSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void OceanSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    OceanSystemComponent::OceanSystemComponent()
    {
        if (OceanInterface::Get() == nullptr)
        {
            OceanInterface::Register(this);
        }
    }

    OceanSystemComponent::~OceanSystemComponent()
    {
        if (OceanInterface::Get() == this)
        {
            OceanInterface::Unregister(this);
        }
    }

    void OceanSystemComponent::Init()
    {
    }

    void OceanSystemComponent::Activate()
    {
        OceanRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void OceanSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        OceanRequestBus::Handler::BusDisconnect();
    }

    void OceanSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace Ocean
