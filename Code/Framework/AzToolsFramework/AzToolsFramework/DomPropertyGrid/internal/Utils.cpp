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

#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/DomPropertyGrid/internal/Utils.h>

namespace AzToolsFramework::DomPropertyGridInternal
{
#if AZ_REFLECTION_PROTOTYPE_ENABLED
    void ConvertAndAddAttribute(AZ::Edit::AttributeArray& attributes, AZ::Crc32 id, const AZ::Reflection::AttributeDataType& value)
    {
        AZStd::visit(
            [&attributes, id](const auto& valueData)
            {
                AddAttribute(attributes, id, valueData);
            }, value);
    }
#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
} // namespace AzToolsFramework

