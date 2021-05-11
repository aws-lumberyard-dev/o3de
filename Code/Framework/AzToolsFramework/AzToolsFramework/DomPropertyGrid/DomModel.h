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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelData.h>

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
        //! Standard element data that can be reused by all models to expand and hide elements.
        AZ::Edit::ElementData m_hiddenElementDescription;
        rapidjson::Document::AllocatorType* m_domAllocator{nullptr};
        AZ::SerializeContext* m_serializeContext{nullptr};
        DomModelEvent m_eventCallback;
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
        DomModelData m_root;
        rapidjson::Value* m_dom{ nullptr };
    };
} // AzToolsFramework
