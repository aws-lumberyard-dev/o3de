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
        virtual ~IDescriber() = default;
        virtual void DescribeBool([[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeInt8([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeInt16([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeInt32([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeInt64([[maybe_unused]] const IAttributes& attributes){};

        //virtual void DescribeUint8([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeUint16([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeUint32([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeUint64([[maybe_unused]] const IAttributes& attributes){};

        //virtual void DescribeFloat([[maybe_unused]] const IAttributes& attributes){};
        //virtual void DescribeDouble([[maybe_unused]] const IAttributes& attributes){};

        virtual void DescribeString([[maybe_unused]] const IAttributes& attributes){};

        virtual void DescribeObjectBegin(
            [[maybe_unused]] AZStd::string_view type,
            [[maybe_unused]] const AZ::TypeId& typeId,
            [[maybe_unused]] const IAttributes& attributes){};
        virtual void DescribeObjectEnd(){};
    };
} //namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
