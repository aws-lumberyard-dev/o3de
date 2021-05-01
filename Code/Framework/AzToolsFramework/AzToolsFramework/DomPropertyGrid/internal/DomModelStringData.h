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
    // Wrapper for a string so the DOM allocator can be stored along with it.
    class DomModelStringData final
    {
    public:
        AZ_TYPE_INFO(AzToolsFramework::DomModelStringData, "{A5F53640-87E8-4454-ACEE-D814A05B806F}");

        DomModelStringData() = default;
        DomModelStringData(rapidjson::Value& value, DomModelContext* context);

        static void Reflect(AZ::ReflectContext* context);

        void CommitToDom(rapidjson::Value& value) const;

    private:
        AZStd::string m_string;
        rapidjson::Document::AllocatorType* m_domAllocator{nullptr};
    };
} // namespace AzToolsFramework
