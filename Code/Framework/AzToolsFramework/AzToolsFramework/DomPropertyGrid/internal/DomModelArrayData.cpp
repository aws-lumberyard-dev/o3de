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
    DomModelArrayData::DomModelArrayData(rapidjson::Value& value)
        : m_domValue(&value)
    {
        AZ_Assert(value.IsArray(), "DomModelArrayData only supports DOM objects.");
    }

    AZStd::vector<AZStd::shared_ptr<DomModelData>>& DomModelArrayData::GetElements()
    {
        return m_elements;
    }

    const AZStd::vector<AZStd::shared_ptr<DomModelData>>& DomModelArrayData::GetElements() const
    {
        return m_elements;
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
                    ->DataElement(0, &DomModelArrayData::m_elements, "Elements", "Storage for the array elements.");
            }
        }
    }
} // namespace AzToolsFramework
