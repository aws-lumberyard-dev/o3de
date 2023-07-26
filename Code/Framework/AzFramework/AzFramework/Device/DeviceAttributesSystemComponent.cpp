/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributesSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AzFramework
{
    DeviceAttributesSystemComponent::DeviceAttributesSystemComponent() = default;
    DeviceAttributesSystemComponent::~DeviceAttributesSystemComponent() = default;

    void DeviceAttributesSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DeviceAttributesSystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DeviceAttributesSystemComponent>(
                    "AzFramework Device Attributes Component", "System component responsible for registering default device attributes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    void DeviceAttributesSystemComponent::Activate()
    {
        // register default attributes
        RegisterDeviceAttribute(AZStd::make_unique<DeviceAttributeDeviceModel>());
    }

    void DeviceAttributesSystemComponent::Deactivate()
    {
        m_deviceAttributeInterfaces.clear();
    }

    bool DeviceAttributesSystemComponent::RegisterDeviceAttribute(AZStd::unique_ptr<DeviceAttributeInterface> deviceAttributeInterface)
    {
        auto deviceAttribute = deviceAttributeInterface->GetDeviceAttribute();
        if (m_deviceAttributeInterfaces.contains(deviceAttribute))
        {
            AZ_Warning(
                "DeviceAttributesSystemComponent", false,
                "Device attribute '%.*s' is already registered, ignoring new registration request.",
                AZ_STRING_ARG(deviceAttribute));
            return false;
        }

        m_deviceAttributeInterfaces.emplace(deviceAttribute, AZStd::move(deviceAttributeInterface) );
        return true;
    }

    void DeviceAttributesSystemComponent::VisitDeviceAttributes(const VisitInterfaceCallback& callback) const
    {
        for (const auto& [deviceAttribue, deviceAttributeInterface] : m_deviceAttributeInterfaces)
        {
            if (!deviceAttributeInterface)
            {
                continue;
            }

            if (!callback(*deviceAttributeInterface.get()))
            {
                return;
            }
        }
    }

    DeviceAttributeInterface* DeviceAttributesSystemComponent::FindDeviceAttribute(AZStd::string_view deviceAttribute) const
    {
        auto itr = m_deviceAttributeInterfaces.find(deviceAttribute);
        return itr != m_deviceAttributeInterfaces.end() ? itr->second.get() : nullptr;
    }

    void DeviceAttributesSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DeviceAttributesSystemComponentService"));
    }

    void DeviceAttributesSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DeviceAttributesSystemComponentService"));
    }

    void DeviceAttributesSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // AzFramework

