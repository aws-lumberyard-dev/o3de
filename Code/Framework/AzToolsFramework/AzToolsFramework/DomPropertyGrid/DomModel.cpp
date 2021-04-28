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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>

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

    class DomModelObjectData
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
        DomModelObjectData(rapidjson::Value& value, rapidjson::Document::AllocatorType& domAllocator)
            : m_domValue(&value)
        {
            AZ_Assert(value.IsObject(), "DomModelObjectData only supports DOM objects.");

            m_container.m_elements.reserve(value.MemberCount());
            for (auto& element : value.GetObject())
            {
                m_container.m_elements.emplace_back(
                    AZStd::string(element.name.GetString(), element.name.GetStringLength()), element.value, domAllocator);
            }
        }

        static void Reflect(AZ::ReflectContext* context)
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

    private:
        DomModelObjectDataContainer m_container;
        rapidjson::Value* m_domValue{nullptr};
    };

    class DomModelArrayData
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelArrayData, "{71C9618D-3C6A-4510-B778-31B3FEBB2F3B}");

        DomModelArrayData() = default;
        DomModelArrayData(rapidjson::Value& value, rapidjson::Document::AllocatorType& domAllocator)
            : m_domValue(&value)
        {
            AZ_Assert(value.IsArray(), "DomModelArrayData only supports DOM objects.");

            auto&& array = value.GetArray();
            size_t size = array.Size();
            m_elements.reserve(size);
            for (size_t i = 0; i < size; ++i)
            {
                m_elements.emplace_back(AZStd::string::format("[%llu]", i), array[i], domAllocator);
            }
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
            {
                serializeContext->Class<DomModelArrayData>()
                    ->Field("Container", &DomModelArrayData::m_elements);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
                {
                    editContext->Class<DomModelArrayData>("DOM Model data for arrays", "Data used to display array.")
                        ->DataElement(0, &DomModelArrayData::m_elements, "Elements", "Storage for the array elements.")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false);
                }
            }
        }

    private:
        AZStd ::vector<DomModelData> m_elements;
        rapidjson::Value* m_domValue{nullptr};
    };

    // Wrapper for a string so the DOM allocator can be stored along with it.
    class DomModelStringData
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelStringData, "{A5F53640-87E8-4454-ACEE-D814A05B806F}");

        DomModelStringData() = default;
        DomModelStringData(rapidjson::Value& value, rapidjson::Document::AllocatorType& domAllocator)
            : m_string(value.GetString(), value.GetStringLength())
            , m_domAllocator(&domAllocator)
        {}

        static void Reflect(AZ::ReflectContext* context)
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

        void CommitToDom(rapidjson::Value& value) const
        {
            value.SetString(m_string.c_str(), aznumeric_caster(m_string.length()), *m_domAllocator);
        }

    private:
        AZStd::string m_string;
        rapidjson::Document::AllocatorType* m_domAllocator{nullptr};
    };

    DomModelData::DomModelData(AZStd::string name, rapidjson::Value& value, rapidjson::Document::AllocatorType& domAllocator)
        : m_name(AZStd::move(name))
        , m_domValue(&value)
        , m_domAllocator(&domAllocator)    
    {
        m_domElement.m_name = m_name.c_str();
        m_domElement.m_description = "A value in the DOM.";
        m_domElement.m_elementId = AZ::Edit::UIHandlers::Default;
        m_domElement.m_serializeClassElement = nullptr;

        if (value.IsBool())
        {
            m_value = value.GetBool();
        }
        else if (value.IsUint64())
        {
            m_value = value.GetUint64();
        }
        else if (value.IsInt64())
        {
            m_value = value.GetInt64();
        }
        else if (value.IsDouble())
        {
            m_value = value.GetDouble();
        }
        else if (value.IsString())
        {
            m_value = DomModelStringData(value, domAllocator);
        }
        else if (value.IsObject())
        {
            m_value = DomModelObjectData(value, domAllocator);
        }
        else if (value.IsArray())
        {
            m_value = DomModelArrayData(value, domAllocator);
        }

        AddAttribute(m_domElement.m_attributes, AZ::Edit::Attributes::ChangeNotify, [this]() { return OnDataChanged(); });
    }

    DomModelData::~DomModelData()
    {
        m_domElement.ClearAttributes();
    }

    const AZ::Edit::ElementData* DomModelData::ProvideEditData(
        const void* handlerPtr, const void* /*elementPtr*/, const AZ::Uuid& /*elementType*/)
    {
        return &(reinterpret_cast<const DomModelData*>(handlerPtr)->m_domElement);
    }

    void DomModelData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelData>()->Field("Value", &DomModelData::m_value);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelData>("", "")
                    ->SetDynamicEditDataProvider(&DomModelData::ProvideEditData)
                    // AZStd::any is a presented as a container. By only expending it and only showing the one container entry, the stored
                    // edit data will be used for the element.
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DomModelData::m_value, "", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    AZ::u32 DomModelData::OnDataChanged() const
    {
        if (m_domValue)
        {
            if (m_value.is<bool>())
            {
                auto value = AZStd::any_cast<bool>(m_value);
                m_domValue->SetBool(value);
            }
            else if (m_value.is<uint64_t>())
            {
                auto value = AZStd::any_cast<uint64_t>(m_value);
                m_domValue->SetUint64(value);
            }
            else if (m_value.is<int64_t>())
            {
                auto value = AZStd::any_cast<int64_t>(m_value);
                m_domValue->SetInt64(value);
            }
            else if (m_value.is<double>())
            {
                auto value = AZStd::any_cast<double>(m_value);
                m_domValue->SetDouble(value);
            }
            else if (m_value.is<DomModelStringData>())
            {
                auto& value = AZStd::any_cast<const DomModelStringData&>(m_value);
                value.CommitToDom(*m_domValue);
            }
        }
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void DomModel::SetDom(rapidjson::Value& dom, rapidjson::Document::AllocatorType& domAllocator)
    {
        m_dom = &dom;
        
        m_elements.clear();
        if (dom.IsObject())
        {
            m_elements.reserve(dom.MemberCount());
            for (auto& [key, value] : dom.GetObject())
            {
                m_elements.emplace_back(AZStd::string(key.GetString(), key.GetStringLength()), value, domAllocator);
            }
        }
    }

    void DomModel::Reflect(AZ::ReflectContext* context)
    {
        DomModelObjectData::Reflect(context);
        DomModelArrayData::Reflect(context);
        DomModelStringData::Reflect(context);
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
