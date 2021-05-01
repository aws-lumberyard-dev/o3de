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

#include <AzToolsFramework/DomPropertyGrid/internal/DomModelArrayData.h>

namespace AzToolsFramework
{
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
            serializeContext->Class<DomModelArrayData>()->Field("Container", &DomModelArrayData::m_elements);

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
} // namespace AzToolsFramework
