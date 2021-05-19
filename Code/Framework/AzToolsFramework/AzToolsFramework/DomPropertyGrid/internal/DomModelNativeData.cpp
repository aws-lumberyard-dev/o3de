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

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelNativeData.h>

namespace AzToolsFramework
{
    DomModelNativeData::DomModelNativeData(
        rapidjson::Value& value, AZStd::string_view path, DomModelContext* context, const AZ::TypeId& targetType)
        : m_context(context)
        , m_path(path)
    {
        AZStd::any instance = m_context->m_serializeContext->CreateAny(targetType);
        if (!instance.empty())
        {
            AZ::JsonSerializationResult::ResultCode result =
                AZ::JsonSerialization::Load(AZStd::any_cast<void>(&instance), targetType, value);
            if (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
            {
                m_object = AZStd::move(instance);
            }
        }
    }

    void DomModelNativeData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DomModelNativeData>()->Field("Object", &DomModelNativeData::m_object);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext(); editContext != nullptr)
            {
                editContext->Class<DomModelNativeData>("DOM Model data for native data", "Data used to display native data using standard RPE.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(0, &DomModelNativeData::m_object, "Data", "Storage for the string.")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void DomModelNativeData::CommitToDom(rapidjson::Value& value) const
    {
        if (!m_object.empty())
        {
            value.SetNull();
            AZ::JsonSerializerSettings settings;
            settings.m_keepDefaults = true;
            [[maybe_unused]] AZ::JsonSerializationResult::ResultCode result =
            AZ::JsonSerialization::Store(
                value, *(m_context->m_domAllocator), AZStd::any_cast<void>(&m_object), nullptr, m_object.type(), settings);
            AZ_Assert(
                result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted,
                "Failed to commit native RPE element to JSON because: ", result.ToString(m_path).c_str());
        }
    }

    bool DomModelNativeData::IsEmpty() const
    {
        return m_object.empty();
    }
} // namespace AzToolsFramework
