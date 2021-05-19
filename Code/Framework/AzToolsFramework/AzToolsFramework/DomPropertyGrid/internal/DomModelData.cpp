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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomDescriber.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelArrayData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelNativeData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelStringData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/Utils.h>

namespace AzToolsFramework
{
    DomModelData::DomModelData(
        AZStd::string name, AZStd::string description, AZStd::string path, rapidjson::Value& value, DomModelContext* context)
        : DomModelData(AZStd::move(name), AZStd::move(description), AZStd::move(path), value, context, AZ::TypeId::CreateNull())
    {
    }

    DomModelData::DomModelData(DomModelData&& rhs) noexcept
    {
        *this = AZStd::move(rhs);
    }

    DomModelData::DomModelData(
        AZStd::string name, AZStd::string description, AZStd::string path, rapidjson::Value& value, DomModelContext* context,
        const AZ::TypeId& targetType)
        : m_name(AZStd::move(name))
        , m_description(AZStd::move(description))
        , m_path(AZStd::move(path))
        , m_domValue(&value)
        , m_context(context)
    {
        using namespace DomPropertyGridInternal;

        m_domElement.m_name = m_name.c_str();
        m_domElement.m_description = m_description.c_str();
        m_domElement.m_elementId = AZ::Edit::UIHandlers::Default;
        m_domElement.m_serializeClassElement = nullptr;

        if (!targetType.IsNull())
        {
            m_value = AZStd::make_any<DomModelNativeData>(value, m_path, context, targetType);
            AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitNativeToDom);
            AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::AutoExpand, true);
            AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
        else
        {
            if (value.IsBool())
            {
                m_value = value.GetBool();
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitBoolToDom);
            }
            else if (value.IsUint64())
            {
                m_value = value.GetUint64();
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitUint64ToDom);
            }
            else if (value.IsInt64())
            {
                m_value = value.GetInt64();
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitInt64ToDom);
            }
            else if (value.IsDouble())
            {
                m_value = value.GetDouble();
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitDoubleToDom);
            }
            else if (value.IsString())
            {
                m_value = AZStd::make_any<DomModelStringData>(value, context);
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitStringToDom);
            }
            else if (value.IsObject())
            {
                m_value = AZStd::make_shared<DomModelObjectData>(value);
                // Doesn't need to write anything back as that will be done by the values contained within.
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::AutoExpand, true);
            }
            else if (value.IsArray())
            {
                m_value = AZStd::make_shared<DomModelArrayData>(value);
                // Doesn't need to write anything back as that will be done by the values contained within.
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    DomModelData::~DomModelData()
    {
        m_domElement.ClearAttributes();
    }

    DomModelData& DomModelData::operator=(DomModelData&& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_value = AZStd::move(rhs.m_value);
            m_description = AZStd::move(rhs.m_description);
            m_name = AZStd::move(rhs.m_name);
            m_path = AZStd::move(rhs.m_path);
            m_targetType = rhs.m_targetType;

            m_domValue = rhs.m_domValue;
            m_context = rhs.m_context;
            rhs.m_domValue = nullptr;
            rhs.m_context = nullptr;

            m_domElement.m_elementId = rhs.m_domElement.m_elementId;
            m_domElement.m_description = m_description.c_str();
            m_domElement.m_name = m_name.c_str();
            m_domElement.m_deprecatedName = rhs.m_domElement.m_deprecatedName;
            m_domElement.m_serializeClassElement = rhs.m_domElement.m_serializeClassElement;
            rhs.m_domElement.m_description = nullptr;
            rhs.m_domElement.m_name = nullptr;
            rhs.m_domElement.m_deprecatedName = nullptr;
            rhs.m_domElement.m_serializeClassElement = nullptr;

            m_domElement.m_attributes = AZStd::move(rhs.m_domElement.m_attributes);
            AZ_Assert(
                rhs.m_domElement.m_attributes.empty(), "Attribute array moved in DomModelData, but moved array still contains values.");
        }
        return *this;
    }

    const AZ::Edit::ElementData* DomModelData::ProvideEditData(
        const void* handlerPtr, const void* /*elementPtr*/, const AZ::Uuid& elementType)
    {
        auto modelData = reinterpret_cast<const DomModelData*>(handlerPtr);
        if (modelData->m_value.is<DomModelNativeData>())
        {
            if (elementType == azrtti_typeid<DomModelNativeData>())
            {
                return &(modelData->m_domElement); 
            }
            else if (elementType == azrtti_typeid<AZStd::any>())
            {
                return &modelData->m_context->m_hiddenElementDescription;
            }
            // Needs to return null instead of the dom element because it should be using any
            // Edit Context information on the wrapped component.
            return nullptr;
        }
        else if (
            elementType == azrtti_typeid<DomModelData>() ||
            elementType == azrtti_typeid<DomModelStringData>() ||
            elementType == azrtti_typeid<AZStd::any>() ||
            elementType == azrtti_typeid<AZStd::shared_ptr<DomModelData>>() ||
            elementType == azrtti_typeid<AZStd::shared_ptr<DomModelArrayData>>() ||
            elementType == azrtti_typeid<AZStd::shared_ptr<DomModelObjectData>>() ||
            elementType == azrtti_typeid<AZStd::vector<AZStd::shared_ptr<DomModelData>>>())
        {
            return &modelData->m_context->m_hiddenElementDescription;
        }
        else
        {
            return &(modelData->m_domElement);
        }
    }

    void DomModelData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelData>()
                ->Field("Value", &DomModelData::m_value);

            serializeContext->RegisterGenericType<AZStd::shared_ptr<DomModelArrayData>>();
            serializeContext->RegisterGenericType<AZStd::shared_ptr<DomModelObjectData>>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelData>("", "")
                    ->SetDynamicEditDataProvider(&DomModelData::ProvideEditData)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DomModelData::m_value, "", "");
            }
        }
    }

    bool DomModelData::IsEmpty() const
    {
        return m_value.empty();
    }

    bool DomModelData::IsValid() const
    {
        auto data = AZStd::any_cast<DomModelNativeData>(&m_value);
        return data ? !data->IsEmpty() : !m_value.empty();
    }

    bool DomModelData::IsObject() const
    {
        return m_value.is<AZStd::shared_ptr<DomModelObjectData>>();
    }

    DomModelObjectData* DomModelData::GetObjectData()
    {
        auto data = AZStd::any_cast<AZStd::shared_ptr<DomModelObjectData>>(&m_value);
        return data != nullptr ? (*data).get() : nullptr;
    }

    const DomModelObjectData* DomModelData::GetObjectData() const
    {
        auto data = AZStd::any_cast<AZStd::shared_ptr<const DomModelObjectData>>(&m_value);
        return data != nullptr ? (*data).get() : nullptr;
    }

    bool DomModelData::IsArray() const
    {
        return m_value.is<AZStd::shared_ptr<DomModelArrayData>>();
    }

    DomModelArrayData* DomModelData::GetArrayData()
    {
        auto data = AZStd::any_cast<AZStd::shared_ptr<DomModelArrayData>>(&m_value);
        return data != nullptr ? (*data).get() : nullptr;
    }

    const DomModelArrayData* DomModelData::GetArrayData() const
    {
        auto data = AZStd::any_cast<AZStd::shared_ptr<const DomModelArrayData>>(&m_value);
        return data != nullptr ? (*data).get() : nullptr;
    }

    AZ::Edit::ElementData& DomModelData::GetDomElement()
    {
        return m_domElement;
    }

    const AZ::Edit::ElementData& DomModelData::GetDomElement() const
    {
        return m_domElement;
    }

    rapidjson::Value* DomModelData::GetDomValue()
    {
        return m_domValue;
    }

    const rapidjson::Value* DomModelData::GetDomValue() const
    {
        return m_domValue;
    }

    const AZStd::string& DomModelData::GetName() const
    {
        return m_name;
    }

    const AZStd::string& DomModelData::GetDescription() const
    {
        return m_description;
    }

    const AZStd::string& DomModelData::GetPath() const
    {
        return m_path;
    }

    AZ::u32 DomModelData::CommitBoolToDom()
    {
        m_domValue->SetBool(AZStd::any_cast<bool>(m_value));
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitUint64ToDom()
    {
        m_domValue->SetUint64(AZStd::any_cast<uint64_t>(m_value));
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitInt64ToDom()
    {
        m_domValue->SetInt64(AZStd::any_cast<int64_t>(m_value));
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitDoubleToDom()
    {
        m_domValue->SetDouble(AZStd::any_cast<double>(m_value));
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitStringToDom()
    {
        AZStd::any_cast<const DomModelStringData&>(m_value).CommitToDom(*m_domValue);
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitNativeToDom()
    {
        AZStd::any_cast<const DomModelNativeData&>(m_value).CommitToDom(*m_domValue);
        m_context->m_eventCallback(DomModelEventType::Replace, m_path);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
} // namespace AzToolsFramework
