/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/any.h>

namespace AzFramework
{
    // DeviceAttributeInterface provides an interface for requesting information about a
    // single device attribute like model, GPU type, RAM amount etc.
    class DeviceAttributeInterface
    {
    public:
        virtual ~DeviceAttributeInterface() = default;

        //! Get the name of the device attribute e.g. gpuMemory, gpuVendor, customAttribute42
        virtual AZStd::string_view GetDeviceAttribute() const = 0;

        //! Get a description of this device attribute, used for help text and eventual UI
        virtual AZStd::string_view GetDescription() const = 0;

        //! Evaluate a rule and return true if there is a match for this device attribute
        virtual bool Evaluate(AZStd::string_view rule) const = 0;

        //! Get the value of this attribute
        virtual AZStd::any GetValue() const = 0;
    };

    // DeviceAttributeRegistrarInterface provides an interface for registering and accessing
    // DeviceAttributeInterfaces which describes a single device attribute.
    class DeviceAttributeRegistrarInterface
    {
    public:
        virtual ~DeviceAttributeRegistrarInterface() = default;

        //! Register a device attribute interface, deviceAttribute must be unique, returns true on success
        virtual bool RegisterDeviceAttribute(AZStd::unique_ptr<DeviceAttributeInterface> deviceAttributeInterface) = 0;

        using VisitInterfaceCallback = AZStd::function<bool(DeviceAttributeInterface&)>;

        //! Visit device attribute interfaces with a callback function
        virtual void VisitDeviceAttributes(const VisitInterfaceCallback&) const = 0;

        //! Find a device attribute by name
        virtual DeviceAttributeInterface* FindDeviceAttribute(AZStd::string_view deviceAttribute) const = 0;
    };
    using DeviceAttributeRegistrar = AZ::Interface<DeviceAttributeRegistrarInterface>;

    // Common device attributes

    // Device Model
    class DeviceAttributeDeviceModel
        : public DeviceAttributeInterface
    {
    public:
        DeviceAttributeDeviceModel();
        AZStd::string_view GetDeviceAttribute() const override;
        AZStd::string_view GetDescription() const override;
        bool Evaluate(AZStd::string_view rule) const override;
        AZStd::any GetValue() const override;
    private:
        AZStd::string m_deviceModel;
    };
} // namespace AzFramework
