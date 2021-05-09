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
#include <AzCore/Reflection/IDescriber.h>
#include <AzCore/Reflection/IVisitor.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace AZ::Reflection
{
    template<typename T>
    struct Reflect
    {
        void operator()([[maybe_unused]] IVisitor& visitor, [[maybe_unused]] T& value) {}
        void operator()([[maybe_unused]] IDescriber& describer){}
    };

    template<typename T>
    void Visit(IVisitor& visitor, T& value)
    {
        Reflect<T>{}(visitor, value);
    }

    template<typename T>
    void Describe(IDescriber& describer)
    {
        Reflect<T>{}(describer);
    }



    template<>
    struct Reflect<int8_t>
    {
        void operator()(IVisitor& visitor, int8_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeInt8(); }
    };

    template<>
    struct Reflect<int16_t>
    {
        void operator()(IVisitor& visitor, int16_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeInt16(); }
    };

    template<>
    struct Reflect<int32_t>
    {
        void operator()(IVisitor& visitor, int32_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeInt32(); }
    };

    template<>
    struct Reflect<int64_t>
    {
        void operator()(IVisitor& visitor, int64_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeInt64(); }
    };

    template<>
    struct Reflect<uint8_t>
    {
        void operator()(IVisitor& visitor, uint8_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeUint8(); }
    };

    template<>
    struct Reflect<uint16_t>
    {
        void operator()(IVisitor& visitor, uint16_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeUint16(); }
    };

    template<>
    struct Reflect<uint32_t>
    {
        void operator()(IVisitor& visitor, uint32_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeUint32(); }
    };

    template<>
    struct Reflect<uint64_t>
    {
        void operator()(IVisitor& visitor, uint64_t& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeUint64(); }
    };

    template<>
    struct Reflect<float>
    {
        void operator()(IVisitor& visitor, float& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeFloat(); }
    };

    template<>
    struct Reflect<double>
    {
        void operator()(IVisitor& visitor, double& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeDouble(); }
    };

    template<>
    struct Reflect<AZStd::string>
    {
        void operator()(IVisitor& visitor, AZStd::string& value) { visitor.Visit(value); }
        void operator()(IDescriber& describer) { describer.DescribeString(); }
    };
}
