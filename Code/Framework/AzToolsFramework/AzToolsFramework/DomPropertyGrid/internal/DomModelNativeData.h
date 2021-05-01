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
    // Wrapper for objects that use the native RPE RPE.
    class DomModelNativeData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelNativeData, "{0CD02E5D-27CD-4C25-9E68-0B9D67A731A5}");

        DomModelNativeData() = default;
        DomModelNativeData(rapidjson::Value& value, AZStd::string_view path, DomModelContext* context, const AZ::TypeId& targetType);

        static void Reflect(AZ::ReflectContext* context);

        void CommitToDom(rapidjson::Value& value) const;

    private:
        AZStd::any m_object;
        AZStd::string_view m_path;
        DomModelContext* m_context{nullptr};
    };
} // namespace AzToolsFramework
