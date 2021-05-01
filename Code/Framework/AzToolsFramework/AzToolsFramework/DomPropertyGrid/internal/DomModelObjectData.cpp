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

#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>

namespace AzToolsFramework
{
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
            serializeContext->Class<DomModelObjectDataContainer>()->Field("Elements", &DomModelObjectDataContainer::m_elements);

            serializeContext->Class<DomModelObjectData>()->Field("Container", &DomModelObjectData::m_container);

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
} // namespace AzToolsFramework
