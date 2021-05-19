/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Reflection/ReflectionConfig.h>

#include <AzCore/Reflection/Attributes.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework::DomPropertyGridInternal
{
    template<typename T>
    static void AddAttribute(AZ::Edit::AttributeArray& attributes, AZ::Crc32 id, T&& value)
    {
        using ValueType = AZ::AttributeContainerType<T>;
        attributes.emplace_back(id, aznew ValueType(AZStd::forward<T>(value)));
    }

    // Needed to make sure the correct ValueType is used.
    template<typename T>
    static void AddAttribute(AZ::Edit::AttributeArray& attributes, AZ::Crc32 id, const T& value)
    {
        using ValueType = AZ::AttributeContainerType<T>;
        attributes.emplace_back(id, aznew ValueType(value));
    }

#if AZ_REFLECTION_PROTOTYPE_ENABLED
    void ConvertAndAddAttribute(AZ::Edit::AttributeArray& attributes, AZ::Crc32 id, const AZ::Reflection::AttributeDataType& value);
#endif
} // namespace AzToolsFramework
