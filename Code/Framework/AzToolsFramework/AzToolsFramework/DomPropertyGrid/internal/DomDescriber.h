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

#include <AzCore/JSON/document.h>
#include <AzCore/Reflection/IDescriber.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AzToolsFramework
{
    class DomDescriber
        : public AZ::Reflection::IDescriber
    {
    public:
        explicit DomDescriber(rapidjson::Value& rootValue);
        ~DomDescriber() override = default;

        void DescribeInt8() override;
        void DescribeInt16() override;
        void DescribeInt32() override;
        void DescribeInt64() override;
        
        void DescribeUint8() override;
        void DescribeUint16() override;
        void DescribeUint32() override;
        void DescribeUint64() override;
        
        void DescribeFloat() override;
        void DescribeDouble() override;
        
        void DescribeString() override;
        
        void DescribeObjectBegin(AZStd::string_view type, const AZ::TypeId& typeId) override;
        void DescribeObjectEnd() override;

    private:
        rapidjson::Value& m_rootValue;
        AZ::StackedString m_path{ AZ::StackedString::Format::JsonPointer };
        size_t m_depth{ 0 };
    };
} // namespace AzToolsFramework
