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

#include <AzCore/Reflection/Attributes.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomDescriber.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/Utils.h>

namespace AzToolsFramework
{
    DomDescriber::DomDescriber(rapidjson::Value& rootValue, DomModelData& rootModel, DomModelContext* context)
        : m_context(context)
    {
        m_modelStack.push(&rootModel);
        m_valueStack.push(&rootValue);
    }

    void DomDescriber::DescribeBool(const AZ::Reflection::IAttributes& attributes)
    {
        // TODO: Figure out the default value for the boolean and pass in true or false.
        AddEntry(rapidjson::kTrueType, "bool", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeInt8(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "int8", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeInt16(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "int16", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeInt32(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "int32", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeInt64(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "int64", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeUint8(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "uint8", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeUint16(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "uint16", AZ_CRC_CE("Variable"), attributes);
    }
    void DomDescriber::DescribeUint32(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "uint32", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeUint64(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "uint64", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeFloat(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "float", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeDouble(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kNumberType, "double", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeString(const AZ::Reflection::IAttributes& attributes)
    {
        AddEntry(rapidjson::kStringType, "string", AZ_CRC_CE("Variable"), attributes);
    }

    void DomDescriber::DescribeObjectBegin(
        [[maybe_unused]] AZStd::string_view type, [[maybe_unused]] const AZ::TypeId& typeId, const AZ::Reflection::IAttributes& attributes)
    {
        const bool isRoot = m_stackDepth == 0;
        DomModelData* model = AddEntry(rapidjson::kObjectType, "Object", AZ_CRC_CE("Class"), attributes, isRoot);

        // Skip the root object. It's already been added and the name will always be empty and not the one passed in.
        if (!isRoot)
        {
            m_path.Push(model->GetName());
            m_valueStack.push(model->GetDomValue());
            m_modelStack.push(model);
        }
        m_stackDepth++;
    }

    void DomDescriber::DescribeObjectEnd()
    {
        m_stackDepth--;
        if (m_stackDepth != 0)
        {
            m_path.Pop();
            m_valueStack.pop();
            m_modelStack.pop();
        }
    }

    DomModelData* DomDescriber::AddEntry(
        rapidjson::Type type, AZStd::string_view defaultName, AZ::Crc32 elementGroup, const AZ::Reflection::IAttributes& attributes, bool isRoot)
    {
        AZStd::string_view name = AZ::Reflection::AttributeReader::GetString(attributes, elementGroup, AZ_CRC_CE("Name"), defaultName);
        AZStd::string_view displayName =
            AZ::Reflection::AttributeReader::GetBool(attributes, AZ_CRC_CE("PropertyGrid"), AZ_CRC_CE("HideName"), false) ? "" : name;
        AZStd::string_view description =
            AZ::Reflection::AttributeReader::GetString(attributes, elementGroup, AZ_CRC_CE("Description"), "");
        AZStd::string_view editor =
            AZ::Reflection::AttributeReader::GetString(attributes, AZ_CRC_CE("PropertyGrid"), AZ_CRC_CE("Editor"), "");

        AZ::ScopedStackedString namePath(m_path, isRoot ? AZStd::string_view{} : name);
        DomModelData* model = AddModelData(displayName, description, isRoot ? *m_valueStack.top() : FindOrAddValue(name, type));
        model->GetDomElement().m_elementId = editor.empty() ? AZ::Edit::UIHandlers::Default : AZ::Crc32(editor);
        AZ::Edit::AttributeArray& elementAttributes = model->GetDomElement().m_attributes;

        // Used to filter out duplicates. The later attributes will closer to the target type.
        AZStd::unordered_map<AZ::Crc32, const AZ::Reflection::AttributeDataType*> filteredAttributes;
        AZ::Reflection::AttributeReader::ListAttributes(attributes,
            [&elementAttributes, &filteredAttributes](AZ::Crc32 group, AZ::Crc32 name, const AZ::Reflection::AttributeDataType& attribute)
            {
                if (group == AZ_CRC_CE("PropertyGrid") && name != AZ_CRC_CE("Editor") && name != AZ_CRC_CE("HideName"))
                {
                    filteredAttributes.insert_or_assign(name, &attribute);
                }
            });
        for (auto&& [nameCrc, attribute] : filteredAttributes)
        {
            DomPropertyGridInternal::ConvertAndAddAttribute(elementAttributes, nameCrc, *attribute);
        }

        return model;
    }

    rapidjson::Value& DomDescriber::FindOrAddValue(AZStd::string_view name, rapidjson::Type type)
    {
        rapidjson::Value* owningValue = m_valueStack.top();
        AZ_Assert(
            owningValue->IsObject(), "Dom Property Grid is unable to find or add value because the provided target isn't a JSON Object.");
        auto it = owningValue->FindMember(rapidjson::StringRef(name.data(), name.length()));
        if (it != owningValue->MemberEnd())
        {
            return it->value;
        }
        else
        {
            rapidjson::Value memberName;
            memberName.SetString(name.data(), name.length(), *(m_context->m_domAllocator));
            owningValue->AddMember(AZStd::move(memberName), rapidjson::Value(type), *(m_context->m_domAllocator));
            return owningValue->FindMember(rapidjson::StringRef(name.data(), name.length()))->value;
        }
    }

    DomModelData* DomDescriber::AddModelData(AZStd::string_view name, AZStd::string_view description, rapidjson::Value& value)
    {
        DomModelData* owner = m_modelStack.top();
        if (owner->IsObject())
        {
            DomModelObjectData* object = owner->GetObjectData();
            auto newModel = AZStd::make_shared<DomModelData>(name, description, m_path.Get(), value, m_context);
            auto result = newModel.get();
            object->GetElements().push_back(AZStd::move(newModel));
            return result;
        }
        else if (owner->IsArray())
        {
            AZ_Assert(false, "Dom Property Grid doesn't currently support defining arrays.");
            return nullptr;
        }
        else
        {
            *owner = DomModelData(name, description, m_path.Get(), value, m_context);
            return owner;
        }
    }
} // namespace AzToolsFramework

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
