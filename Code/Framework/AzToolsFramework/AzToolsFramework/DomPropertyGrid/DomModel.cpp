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

#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelNativeData.h>

namespace AzToolsFramework
{
    template<typename T>
    static void AddAttribute(AZ::Edit::AttributeArray& attributes, const AZ::Crc32& id, T&& value)
    {
        using ValueType = AZ::AttributeContainerType<T>;
        attributes.emplace_back(id, aznew ValueType(AZStd::forward<T>(value)));
    }

    template<typename T>
    static void AddAttribute(AZ::Edit::AttributeArray& attributes, const AZ::Crc32& id, const T& value)
    {
        using ValueType = AZ::AttributeContainerType<T>;
        attributes.emplace_back(id, aznew ValueType(value));
    }


    //
    // DomModelObjectData
    //
    
    DomModelObjectData::DomModelObjectData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context)
        : m_domValue(&value)
    {
        AZ_Assert(value.IsObject(), "DomModelObjectData only supports DOM objects.");

        m_container.m_elements.reserve(value.MemberCount());
        for (auto& element : value.GetObject())
        {
            AZStd::string elementPath = path;
            AZStd::string name(element.name.GetString(), element.name.GetStringLength());
            if (elementPath.back() != '/')
            {
                elementPath += '/';
            }
            elementPath += name;
            m_container.m_elements.emplace_back(AZStd::move(name), AZStd::move(elementPath), element.value, context);
        }
    }

    void DomModelObjectData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelObjectDataContainer>()
                ->Field("Elements", &DomModelObjectDataContainer::m_elements);

            serializeContext->Class<DomModelObjectData>()
                ->Field("Container", &DomModelObjectData::m_container);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext
                    ->Class<DomModelObjectDataContainer>(
                        "DOM Model data elements for object", "Storage for the individual object elements.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(0, &DomModelObjectDataContainer::m_elements, "Elements array", "Storage for the elements.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);

                editContext->Class<DomModelObjectData>("DOM Model data for objects", "Data used to display objects.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &DomModelObjectData::m_container, "Container", "Storage for the object elements.");
            }
        }
    }


    //
    // DomModelArrayData
    //

    DomModelArrayData::DomModelArrayData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context)
        : m_domValue(&value)
    {
        AZ_Assert(value.IsArray(), "DomModelArrayData only supports DOM objects.");

        auto&& array = value.GetArray();
        size_t size = array.Size();
        m_elements.reserve(size);
        for (size_t i = 0; i < size; ++i)
        {
            AZStd::string name = AZStd::string::format("[%llu]", i);
            AZStd::string elementPath = path;
            if (elementPath.back() != '/')
            {
                elementPath += '/';
            }
            elementPath += AZStd::to_string(i);
            m_elements.emplace_back(AZStd::move(name), AZStd::move(elementPath), array[i], context);
        }
    }

    void DomModelArrayData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelArrayData>()
                ->Field("Container", &DomModelArrayData::m_elements);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelArrayData>("DOM Model data for arrays", "Data used to display array.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &DomModelArrayData::m_elements, "Elements", "Storage for the array elements.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false);
            }
        }
    }


    //
    // DomModelStringData
    //

    DomModelStringData::DomModelStringData(rapidjson::Value& value, DomModelContext* context)
        : m_string(value.GetString(), value.GetStringLength())
        , m_domAllocator(context->m_domAllocator)
    {}

    void DomModelStringData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelStringData>()->Field("Data", &DomModelStringData::m_string);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelStringData>("DOM Model data for strings", "Data used to display strings.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &DomModelStringData::m_string, "Data", "Storage for the string.");
            }
        }
    }

    void DomModelStringData::CommitToDom(rapidjson::Value& value) const
    {
        value.SetString(m_string.c_str(), aznumeric_caster(m_string.length()), *m_domAllocator);
    }


    //
    // DomModelData
    //

    DomModelData::DomModelData(AZStd::string name, AZStd::string path, rapidjson::Value& value, DomModelContext* context)
        : DomModelData(AZStd::move(name), AZStd::move(path), value, context, AZ::TypeId::CreateNull())
    {}

    DomModelData::DomModelData(
        AZStd::string name, AZStd::string path, rapidjson::Value& value, DomModelContext* context, const AZ::TypeId& targetType)
        : m_name(AZStd::move(name))
        , m_path(AZStd::move(path))
        , m_domValue(&value)
        , m_context(context)
    {
        m_domElement.m_name = m_name.c_str();
        m_domElement.m_description = "A value in the DOM.";
        m_domElement.m_elementId = AZ::Edit::UIHandlers::Default;
        m_domElement.m_serializeClassElement = nullptr;

        if (!targetType.IsNull())
        {
            m_value = DomModelNativeData(value, m_path, context, targetType);
            AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitNativeToDom);
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
                m_value = DomModelStringData(value, context);
                AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, &DomModelData::CommitStringToDom);
            }
            else if (value.IsObject())
            {
                m_value = DomModelObjectData(value, m_path, context);
                // Doesn't need to write anything back as that will be done by the values contained within.
            }
            else if (value.IsArray())
            {
                m_value = DomModelArrayData(value, m_path, context);
                // Doesn't need to write anything back as that will be done by the values contained within.
            }
        }
    }

    DomModelData::~DomModelData()
    {
        m_domElement.ClearAttributes();
    }

    const AZ::Edit::ElementData* DomModelData::ProvideEditData(
        const void* handlerPtr, const void* /*elementPtr*/, const AZ::Uuid& elementType)
    {
        auto modelData = reinterpret_cast<const DomModelData*>(handlerPtr);
        return (modelData->m_value.is<DomModelNativeData>() && elementType != azrtti_typeid<DomModelNativeData>()) ? nullptr : &(modelData->m_domElement);
    }

    void DomModelData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelData>()
                ->Field("Value", &DomModelData::m_value);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelData>("", "")
                    ->SetDynamicEditDataProvider(&DomModelData::ProvideEditData)
                    // AZStd::any is a presented as a container. By expending it and only showing the one container entry, the stored
                    // edit data will be used for the element.
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DomModelData::m_value, "", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    AZ::u32 DomModelData::CommitBoolToDom()
    {
        m_domValue->SetBool(AZStd::any_cast<bool>(m_value));
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitUint64ToDom()
    {
        m_domValue->SetUint64(AZStd::any_cast<uint64_t>(m_value));
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitInt64ToDom()
    {
        m_domValue->SetInt64(AZStd::any_cast<int64_t>(m_value));
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitDoubleToDom()
    {
        m_domValue->SetDouble(AZStd::any_cast<double>(m_value));
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitStringToDom()
    {
        AZStd::any_cast<const DomModelStringData&>(m_value).CommitToDom(*m_domValue);
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 DomModelData::CommitNativeToDom()
    {
        AZStd::any_cast<const DomModelNativeData&>(m_value).CommitToDom(*m_domValue);
        m_context->m_eventCallback(DomModelEventType::Add, m_path);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }


    //
    // DomModel
    //

    DomModel::DomModel()
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_context.m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(m_context.m_serializeContext, "Unable to retrieve serialize context.");
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator)
    {
        SetDom(dom, domAllocator, AZ::TypeId::CreateNull());
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, DomModelEvent eventCallback)
    {
        SetDom(dom, domAllocator, AZ::TypeId::CreateNull(), AZStd::move(eventCallback));
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType)
    {
        SetDom(dom, domAllocator, targetType, [](DomModelEventType, const AZStd::string_view&) {});
    }

    void DomModel::SetDom(
        rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator, const AZ::TypeId& targetType, DomModelEvent eventCallback)
    {
        m_dom = &dom;
        m_context.m_domAllocator = &domAllocator;
        m_context.m_eventCallback = AZStd::move(eventCallback);

        // TODO: Turn this into a single entry. The first attempt at this crashed though in the value changed callback.
        m_elements.clear();
        m_elements.emplace_back("", "/", dom, &m_context, targetType);
    }

    void DomModel::Reflect(AZ::ReflectContext* context)
    {
        DomModelObjectData::Reflect(context);
        DomModelArrayData::Reflect(context);
        DomModelStringData::Reflect(context);
        DomModelNativeData::Reflect(context);
        DomModelData::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModel>()
                ->Field("Elements", &DomModel::m_elements);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModel>("DOM model", "Data model use to interact with a DOM.")
                    ->DataElement(0, &DomModel::m_elements, "Elements", "Data models for all values in the DOM.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }
} // namespace AzToolsFramework
