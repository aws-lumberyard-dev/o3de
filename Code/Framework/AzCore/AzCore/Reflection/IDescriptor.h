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
        virtual void DescribeBool([[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeInt8([[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeString([[maybe_unused]] const IAttributes& attributes){};

        virtual void DescribeObjectBegin(
            [[maybe_unused]] AZStd::string_view type,
            [[maybe_unused]] const AZ::TypeId& typeId,
            [[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeObjectEnd(){};

        template<typename>
        void Describe([[maybe_unused]] const IAttributes& attributes)
        {
            return;
        }
    };
} //namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
