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

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelData.h>

namespace AzToolsFramework
{
    struct DomModelContext;

    class DomModelObjectData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelObjectData, "{9AD75854-8397-470D-808C-C979BF925B3B}");

        DomModelObjectData() = default;
        explicit DomModelObjectData(rapidjson::Value& value);

        AZStd::vector<AZStd::shared_ptr<DomModelData>>& GetElements();
        const AZStd::vector<AZStd::shared_ptr<DomModelData>>& GetElements() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AZStd::shared_ptr<DomModelData>> m_elements;
        rapidjson::Value* m_domValue{nullptr};
    };
} // namespace AzToolsFramework
