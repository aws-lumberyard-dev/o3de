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

#include <AzToolsFramework/DomPropertyGrid/internal/DomModelStringData.h>

namespace AzToolsFramework
{
    DomModelStringData::DomModelStringData(rapidjson::Value& value, DomModelContext* context)
        : m_string(value.GetString(), value.GetStringLength())
        , m_domAllocator(context->m_domAllocator)
    {
    }

    void DomModelStringData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelStringData>()->Field("Data", &DomModelStringData::m_string);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelStringData>("DOM Model data for strings", "Data used to display strings.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &DomModelStringData::m_string, "Data", "Storage for the string.");
            }
        }
    }

    void DomModelStringData::CommitToDom(rapidjson::Value& value) const
    {
        value.SetString(m_string.c_str(), aznumeric_caster(m_string.length()), *m_domAllocator);
    }
} // namespace AzToolsFramework
