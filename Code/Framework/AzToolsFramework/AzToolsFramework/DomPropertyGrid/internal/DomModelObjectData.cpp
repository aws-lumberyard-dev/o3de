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
    DomModelObjectData::DomModelObjectData(rapidjson::Value& value)
        : m_domValue(&value)
    {
        AZ_Assert(value.IsObject(), "DomModelObjectData only supports DOM objects.");
    }

    AZStd::vector<AZStd::shared_ptr<DomModelData>>& DomModelObjectData::GetElements()
    {
        return m_elements;
    }

    const AZStd::vector<AZStd::shared_ptr<DomModelData>>& DomModelObjectData::GetElements() const
    {
        return m_elements;
    }

    void DomModelObjectData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelObjectData>()
                ->Field("Elements", &DomModelObjectData::m_elements);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelObjectData>("DOM Model data for objects", "Data used to display objects.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &DomModelObjectData::m_elements, "Elements", "Storage for the object elements.");
            }
        }
    }
} // namespace AzToolsFramework
