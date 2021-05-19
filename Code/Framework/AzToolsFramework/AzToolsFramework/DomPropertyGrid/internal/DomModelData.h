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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework
{
    struct DomModelContext;
    class DomModelObjectData;
    class DomModelArrayData;

    class DomModelData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelData, "{D9D86827-12FA-4A7F-841E-A5E85EE33ADF}");

        DomModelData() = default;
        DomModelData(const DomModelData&) = delete;
        DomModelData(DomModelData&& rhs) noexcept;

        DomModelData(AZStd::string name, AZStd::string description, AZStd::string path, rapidjson::Value& value, DomModelContext* context);
        DomModelData(
            AZStd::string name, AZStd::string description, AZStd::string path, rapidjson::Value& value, DomModelContext* context,
            const AZ::TypeId& targetType);
        ~DomModelData();

        DomModelData& operator=(const DomModelData&) = delete;
        DomModelData& operator=(DomModelData&& rhs) noexcept;

        static const AZ::Edit::ElementData* ProvideEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
        static void Reflect(AZ::ReflectContext* context);

        bool IsEmpty() const;
        bool IsValid() const;

        bool IsObject() const;
        DomModelObjectData* GetObjectData();
        const DomModelObjectData* GetObjectData() const;

        bool IsArray() const;
        DomModelArrayData* GetArrayData();
        const DomModelArrayData* GetArrayData() const;

        AZ::Edit::ElementData& GetDomElement();
        const AZ::Edit::ElementData& GetDomElement() const;
        rapidjson::Value* GetDomValue();
        const rapidjson::Value* GetDomValue() const;

        const AZStd::string& GetName() const;
        const AZStd::string& GetDescription() const;
        const AZStd::string& GetPath() const;

    private:
        AZ::u32 CommitBoolToDom();
        AZ::u32 CommitUint64ToDom();
        AZ::u32 CommitInt64ToDom();
        AZ::u32 CommitDoubleToDom();
        AZ::u32 CommitStringToDom();
        AZ::u32 CommitNativeToDom();

        AZ::Edit::ElementData m_domElement;
        AZStd::any m_value;
        AZStd::string m_name;
        AZStd::string m_description;
        AZStd::string m_path;
        AZ::TypeId m_targetType;
        rapidjson::Value* m_domValue{nullptr};
        DomModelContext* m_context{nullptr};
    };
} // namespace AzToolsFramework
