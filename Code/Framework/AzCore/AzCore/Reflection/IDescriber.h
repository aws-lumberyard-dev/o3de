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

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ::Reflection
{
    class IDescriber
    {
    public:
        virtual ~IDescriber() = default;

        virtual void DescribeInt8(){};
        virtual void DescribeInt16(){};
        virtual void DescribeInt32(){};
        virtual void DescribeInt64(){};

        virtual void DescribeUint8(){};
        virtual void DescribeUint16(){};
        virtual void DescribeUint32(){};
        virtual void DescribeUint64(){};

        virtual void DescribeFloat(){};
        virtual void DescribeDouble(){};

        virtual void DescribeString(){};

        virtual void DescribeObjectBegin([[maybe_unused]] AZStd::string_view type, [[maybe_unused]] const AZ::TypeId& typeId){};
        virtual void DescribeObjectEnd(){};
    };
}
