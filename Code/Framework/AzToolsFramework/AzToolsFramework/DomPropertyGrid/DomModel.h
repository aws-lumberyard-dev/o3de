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
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AzToolsFramework
{
    enum class DomModelEventType
    {
        Replace,
        Add,
        Remove
    };

    using DomModelEvent = AZStd::function<void(DomModelEventType eventType, AZStd::string_view path)>;

    struct DomModelContext final
    {
        rapidjson::Document::AllocatorType* m_domAllocator{nullptr};
        AZ::SerializeContext* m_serializeContext{nullptr};
        DomModelEvent m_eventCallback;
    };


    class DomModelData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelData, "{D9D86827-12FA-4A7F-841E-A5E85EE33ADF}");

        DomModelData() = default;
        DomModelData(AZStd::string name, AZStd::string path, rapidjson::Value& value, DomModelContext* context);
        DomModelData(
            AZStd::string name, AZStd::string path, rapidjson::Value& value, DomModelContext* context, const AZ::TypeId& targetType);
        ~DomModelData();

        static const AZ::Edit::ElementData* ProvideEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);
        static void Reflect(AZ::ReflectContext* context);

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
        AZStd::string m_path;
        AZ::TypeId m_targetType;
        rapidjson::Value* m_domValue{nullptr};
        DomModelContext* m_context{nullptr};
    };

    // Wrapper for a string so the DOM allocator can be stored along with it.
    class DomModelStringData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelStringData, "{A5F53640-87E8-4454-ACEE-D814A05B806F}");

        DomModelStringData() = default;
        DomModelStringData(rapidjson::Value& value, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

        void CommitToDom(rapidjson::Value& value) const;

    private:
        AZStd::string m_string;
        rapidjson::Document::AllocatorType* m_domAllocator{nullptr};
    };

    // Wrapper for objects that use the native RPE model.
    class DomModelNativeData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelNativeData, "{0CD02E5D-27CD-4C25-9E68-0B9D67A731A5}");

        DomModelNativeData() = default;
        DomModelNativeData(rapidjson::Value& value, DomModelContext* context, const AZ::TypeId& targetType);

        static void Reflect(AZ::ReflectContext* context);

        void CommitToDom(rapidjson::Value& value) const;

    private:
        AZStd::any m_object;
        DomModelContext* m_context{nullptr};
    };

    class DomModelObjectData final
    {
    public:
        // Wrapper to force the RPE to not overly aggressively collapse children upwards.
        struct DomModelObjectDataContainer
        {
            AZ_TYPE_INFO(AzToolsFramework::DomModelObjectDataContainer, "{92514586-25E3-4922-A021-69643524C5E6}");
            AZStd::vector<DomModelData> m_elements;
        };

        AZ_TYPE_INFO(AzToolsFramework::DomModelObjectData, "{9AD75854-8397-470D-808C-C979BF925B3B}");

        DomModelObjectData() = default;
        DomModelObjectData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

    private:
        DomModelObjectDataContainer m_container;
        rapidjson::Value* m_domValue{nullptr};
    };

    class DomModelArrayData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelArrayData, "{71C9618D-3C6A-4510-B778-31B3FEBB2F3B}");

        DomModelArrayData() = default;
        DomModelArrayData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<DomModelData> m_elements;
        rapidjson::Value* m_domValue{nullptr};
    };

    class DomModel final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModel, "{2A7F861F-E65D-4EB0-984C-A633782BCBA2}");

        DomModel();
        void SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator);
        void SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, DomModelEvent eventCallback);
        void SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType);
        void SetDom(
            rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType,
            DomModelEvent eventCallback);

        static void Reflect(AZ::ReflectContext* context);

    private:
        DomModelContext m_context;
        AZStd::vector<DomModelData> m_elements;
        rapidjson::Value* m_dom{ nullptr };
    };
} // AzToolsFramework
