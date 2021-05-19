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
#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/JSON/document.h>
#include <AzCore/Reflection/IDescriber.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework
{
    class DomModelData;
    struct DomModelContext;

    class DomDescriber
        : public AZ::Reflection::IDescriber
    {
    public:
        DomDescriber(rapidjson::Value& rootValue, DomModelData& rootModel, DomModelContext* context);
        ~DomDescriber() override = default;

        void DescribeBool(const AZ::Reflection::IAttributes& attributes) override;

        void DescribeInt8(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeInt16(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeInt32(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeInt64(const AZ::Reflection::IAttributes& attributes) override;
        
        void DescribeUint8(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeUint16(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeUint32(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeUint64(const AZ::Reflection::IAttributes& attributes) override;
        
        void DescribeFloat(const AZ::Reflection::IAttributes& attributes) override;
        void DescribeDouble(const AZ::Reflection::IAttributes& attributes) override;
        
        void DescribeString(const AZ::Reflection::IAttributes& attributes) override;
        
        void DescribeObjectBegin(AZStd::string_view type, const AZ::TypeId& typeId, const AZ::Reflection::IAttributes& attributes) override;
        void DescribeObjectEnd() override;

    private:
        DomModelData* AddEntry(
            rapidjson::Type type, AZStd::string_view defaultName, AZ::Crc32 elementGroup, const AZ::Reflection::IAttributes& attributes,
            bool isRoot = false);
        DomModelData* AddModelData(AZStd::string_view name, AZStd::string_view description, rapidjson::Value& value);
        rapidjson::Value& FindOrAddValue(AZStd::string_view name, rapidjson::Type type);

        AZStd::stack<DomModelData*> m_modelStack;
        AZStd::stack<rapidjson::Value*> m_valueStack;
        AZ::StackedString m_path{ AZ::StackedString::Format::JsonPointer };
        DomModelContext* m_context;
        size_t m_stackDepth{0};
    };
} // namespace AzToolsFramework

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
