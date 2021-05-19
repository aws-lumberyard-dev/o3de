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

#include <AzCore/Reflection/ReflectionConfig.h>
#if AZ_REFLECTION_PROTOTYPE_ENABLED
#include <AzCore/Reflection/IDescriber.h>
#endif
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomBuilder.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomDescriber.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelArrayData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>

namespace AzToolsFramework
{
    bool DomBuilder::BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context)
    {
        root = DomModelData("", "", "", dom, context);
        if (dom.IsObject() || dom.IsArray())
        {
            AZ::StackedString path(AZ::StackedString::Format::JsonPointer);
            return BuildFromDom(root, dom, context, path);
        }
        else
        {
            // Root type is a plain type so there's nothing to recurse into.
            return true;
        }
    }

    bool DomBuilder::BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context, const AZ::TypeId& targetType)
    {
#if AZ_REFLECTION_PROTOTYPE_ENABLED
        DomDescriber describer(dom, root, context);
        return Describe(describer, root, dom, context, targetType);
#else
        root = DomModelData("", "", "", dom, context, targetType);
        return true;
#endif
    }

    bool DomBuilder::BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context, AZ::StackedString& path)
    {
        if (dom.IsObject())
        {
            bool result = true;

            DomModelObjectData* data = root.GetObjectData();
            AZ_Assert(data, "Dom value in DomBuilder is an object, but stored type doesn't contain object data.");
            data->GetElements().reserve(data->GetElements().size() + dom.MemberCount());
            for (auto& element : dom.GetObject())
            {
                AZStd::string elementName = AZStd::string(element.name.GetString(), element.name.GetStringLength());
                AZ::ScopedStackedString entryPath(path, elementName);

                auto elementData = AZStd::make_shared<DomModelData>(AZStd::move(elementName), "", path.Get(), element.value, context);
                if (element.value.IsObject() || element.value.IsArray())
                {
                    result = BuildFromDom(*elementData, element.value, context, path) && result;
                }
                data->GetElements().push_back(AZStd::move(elementData));
            }
            return result;
        }
        else if (dom.IsArray())
        {
            bool result = true;

            DomModelArrayData* data = root.GetArrayData();
            AZ_Assert(data, "Dom value in DomBuilder is an object, but stored type doesn't contain object data.");
            data->GetElements().reserve(data->GetElements().size() + dom.Size());
            size_t index = 0;
            for (auto& element : dom.GetArray())
            {
                AZ::ScopedStackedString entryPath(path, index);

                auto elementData = AZStd::make_shared<DomModelData>(AZStd::to_string(index), "", path.Get(), element, context);
                if (element.IsObject() || element.IsArray())
                {
                    result = BuildFromDom(*elementData, element, context, path) && result;
                }
                data->GetElements().push_back(AZStd::move(elementData));

                index++;
            }
            return result;
        }
        else
        {
            AZ_Assert(false, "BuildFromDom recursion called with a value that's not an object or array.");
            return false;
        }
    }

#if AZ_REFLECTION_PROTOTYPE_ENABLED
    bool DomBuilder::Describe(
        AZ::Reflection::IDescriber& describer, DomModelData& root, rapidjson::Value& dom, DomModelContext* context,
        const AZ::TypeId& targetType)
    {
        bool result = false;
        const AZ::SerializeContext::ClassData* targetClass = context->m_serializeContext->FindClassData(targetType);
        if (targetClass)
        {
            AZ::Attribute* describerAttribute = targetClass->FindAttribute(AZ_CRC_CE("Describer"));
            if (describerAttribute)
            {
                using DecriberFunction = void (*)(AZ::Reflection::IDescriber & describer);
                auto invocableDescriber = azrtti_cast<AZ::AttributeInvocable<DecriberFunction>*>(describerAttribute);
                if (invocableDescriber)
                {
                    (*invocableDescriber)(describer);
                    result = true;
                }
            }
            else if (targetClass->m_editData)
            {
                bool hasDataElements = false;
                for (AZ::Edit::ElementData& element : targetClass->m_editData->m_elements)
                {
                    if (!element.IsClassElement())
                    {
                        hasDataElements = true;
                        break;
                    }
                }

                if (hasDataElements)
                {
                    AZ_TracePrintf(
                        "Dom Model", "Need to revert to native RPE for: %s.\n", targetClass->m_typeId.ToString<AZStd::string>().c_str());
                    root = DomModelData("", "", "", dom, context, targetType);
                    result = root.IsValid();
                }
            }
            for (const AZ::SerializeContext::ClassElement& element : targetClass->m_elements)
            {
                if (element.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
                {
                    result = Describe(describer, root, dom, context, element.m_typeId) || result;
                }
            }
        }
        return result;
    }
#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
} // namespace AzToolsFramework
