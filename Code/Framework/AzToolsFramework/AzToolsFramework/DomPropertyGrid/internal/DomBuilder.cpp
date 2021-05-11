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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzToolsFramework/DomPropertyGrid/DomModel.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomBuilder.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelArrayData.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelObjectData.h>

namespace AzToolsFramework
{
    bool DomBuilder::BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context)
    {
        root = DomModelData("", "", dom, context);
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

                auto elementData = AZStd::make_shared<DomModelData>(AZStd::move(elementName), path.Get(), element.value, context);
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

                auto elementData = AZStd::make_shared<DomModelData>(AZStd::to_string(index), path.Get(), element, context);
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
} // namespace AzToolsFramework
