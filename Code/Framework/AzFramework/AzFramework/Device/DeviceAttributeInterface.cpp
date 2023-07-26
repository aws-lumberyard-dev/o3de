/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Device/DeviceAttributesSystemComponent.h>

namespace AzFramework
{
    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        // TODO init device model
        m_deviceModel = "DeviceModel";
    }

    AZStd::string_view DeviceAttributeDeviceModel::GetDeviceAttribute() const
    {
        return m_deviceModel;
    }

    AZStd::string_view DeviceAttributeDeviceModel::GetDescription() const
    {
        return "Device model attribute";
    }

    bool DeviceAttributeDeviceModel::Evaluate(AZStd::string_view rule) const
    {
        // TODO evaluate if our device model matches the rule
        return rule.empty() ? false : true;
    }

    AZStd::any DeviceAttributeDeviceModel::GetValue() const
    {
        return AZStd::any(m_deviceModel);
    }
} // namespace AzFramework

