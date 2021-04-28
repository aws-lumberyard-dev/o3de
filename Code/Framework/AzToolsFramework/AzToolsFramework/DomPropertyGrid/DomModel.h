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
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    class DomModelData
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelData, "{D9D86827-12FA-4A7F-841E-A5E85EE33ADF}");

        DomModelData() = default;
        DomModelData(AZStd::string name, rapidjson::Value& value, rapidjson::Document::AllocatorType& domAllocator);
        ~DomModelData();

        static const AZ::Edit::ElementData* ProvideEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 OnDataChanged() const;

    private:
        AZStd::any m_value;
        AZStd::string m_name;
        AZ::Edit::ElementData m_domElement;
        rapidjson::Value* m_domValue{ nullptr };
        rapidjson::Document::AllocatorType* m_domAllocator{ nullptr };
    };

    class DomModel
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModel, "{2A7F861F-E65D-4EB0-984C-A633782BCBA2}");

        void SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<DomModelData> m_elements;
        rapidjson::Value* m_dom{ nullptr };
    };
} // AzToolsFramework
