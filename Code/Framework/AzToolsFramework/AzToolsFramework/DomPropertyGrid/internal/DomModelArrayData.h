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
    class DomModelArrayData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelArrayData, "{71C9618D-3C6A-4510-B778-31B3FEBB2F3B}");

        DomModelArrayData() = default;
        DomModelArrayData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<DomModelData> m_elements;
        rapidjson::Value* m_domValue{nullptr};
    };
} // namespace AzToolsFramework
