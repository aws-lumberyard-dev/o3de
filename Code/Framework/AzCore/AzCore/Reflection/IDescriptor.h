/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Reflection/ReflectionConfig.h>
#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/base.h>
#include <AzCore/Reflection/Attributes.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ::Reflection
{
    class IDescriber
    {
    public:
        AZ_RTTI(IDescriber, "{16F46700-7B05-45DE-8DC0-8C3E9A36C5B0}");
        virtual ~IDescriber() = default;
        virtual void DescribeBool([[maybe_unused]] const IAttributes& attributes) = 0;
        virtual void DescribeInt8([[maybe_unused]] const IAttributes& attributes) = 0;
        virtual void DescribeString([[maybe_unused]] const IAttributes& attributes) = 0;

        virtual void DescribeObjectBegin(
            [[maybe_unused]] AZStd::string_view type,
            [[maybe_unused]] const AZ::TypeId& typeId,
            [[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeObjectEnd() = 0;

        template<typename T>
        void Describe([[maybe_unused]] const IAttributes& attributes)
        {
            if constexpr (AZStd::is_same_v<int, T>)
            {
                DescribeInt8(attributes);
            }
            else if constexpr (AZStd::is_same_v<bool, T>)
            {
                DescribeBool(attributes);
            }
            else if constexpr (AZStd::is_same_v<AZStd::string, T>)
            {
                DescribeString(attributes);
            }
            return;
        }
    };
} //namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
