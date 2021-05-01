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

#include <AzToolsFramework/DomPropertyGrid/DomModel.h>

namespace AzToolsFramework
{
    class DomModelObjectData final
    {
    public:
        // Wrapper to force the RPE to not overly aggressively collapse children upwards.
        struct DomModelObjectDataContainer
        {
            AZ_TYPE_INFO(AzToolsFramework::DomModelObjectDataContainer, "{92514586-25E3-4922-A021-69643524C5E6}");
            AZStd::vector<DomModelData> m_elements;
        };

        AZ_TYPE_INFO(AzToolsFramework::DomModelObjectData, "{9AD75854-8397-470D-808C-C979BF925B3B}");

        DomModelObjectData() = default;
        DomModelObjectData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

    private:
        DomModelObjectDataContainer m_container;
        rapidjson::Value* m_domValue{nullptr};
    };
} // namespace AzToolsFramework
