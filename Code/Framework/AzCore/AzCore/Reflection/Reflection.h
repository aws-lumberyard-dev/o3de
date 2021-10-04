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
#include <AzCore/Reflection/IDescriptor.h>
#include <AzCore/Reflection/IVisitor.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace AZ::Reflection
{
    template<typename T>
    struct Reflect
    {
        void operator()([[maybe_unused]] IVisitor& visitor, [[maybe_unused]] T& value, [[maybe_unused]] const IAttributes& attributes)
        {
        }
        void operator()([[maybe_unused]] IDescriber& describer, [[maybe_unused]] const IAttributes& attributes)
        {
        }
    };

    template<typename T>
    void Visit(IVisitor& visitor, T& value, const IAttributes& attributes = Attributes<0>())
    {
        Reflect<T>{}(visitor, value, attributes);
    }

    template<typename T>
    void Describe(IDescriber& describer, const IAttributes& attributes = Attributes<0>())
    {
        Reflect<T>{}(describer, attributes);
    }

    template<>
    struct Reflect<bool>
    {
        void operator()(IVisitor& visitor, bool& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeBool(attributes);
        }
    };

    template<>
    struct Reflect<int8_t>
    {
        void operator()(IVisitor& visitor, int8_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeInt8(attributes);
        }
    };

    template<>
    struct Reflect<AZStd::string>
    {
        void operator()(IVisitor& visitor, AZStd::string& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeString(attributes);
        }
    };

    /*
    template<>
    struct Reflect<int16_t>
    {
        void operator()(IVisitor& visitor, int16_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeInt16(attributes);
        }
    };

    template<>
    struct Reflect<int32_t>
    {
        void operator()(IVisitor& visitor, int32_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeInt32(attributes);
        }
    };

    template<>
    struct Reflect<int64_t>
    {
        void operator()(IVisitor& visitor, int64_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeInt64(attributes);
        }
    };

    template<>
    struct Reflect<uint8_t>
    {
        void operator()(IVisitor& visitor, uint8_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeUint8(attributes);
        }
    };

    template<>
    struct Reflect<uint16_t>
    {
        void operator()(IVisitor& visitor, uint16_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeUint16(attributes);
        }
    };

    template<>
    struct Reflect<uint32_t>
    {
        void operator()(IVisitor& visitor, uint32_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeUint32(attributes);
        }
    };

    template<>
    struct Reflect<uint64_t>
    {
        void operator()(IVisitor& visitor, uint64_t& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeUint64(attributes);
        }
    };

    template<>
    struct Reflect<float>
    {
        void operator()(IVisitor& visitor, float& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeFloat(attributes);
        }
    };

    template<>
    struct Reflect<double>
    {
        void operator()(IVisitor& visitor, double& value, const IAttributes& attributes)
        {
            visitor.Visit(value, attributes);
        }
        void operator()(IDescriber& describer, const IAttributes& attributes)
        {
            describer.DescribeDouble(attributes);
        }
    };
    */
}//namespace AZ::Reflection
#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
