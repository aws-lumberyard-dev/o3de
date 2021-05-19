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

#include <AzCore/Reflection/ReflectionConfig.h>

#include <AzCore/JSON/document.h>
#include <AzToolsFramework/DomPropertyGrid/internal/DomModelData.h>

namespace AZ
{
    class StackedString;

    namespace Reflection
    {
        class IDescriber;
    }
}

namespace AzToolsFramework
{
    struct DomModelContext;

    class DomBuilder
    {
    public:
        static bool BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context);
        static bool BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context, const AZ::TypeId& targetType);

    private:
        static bool BuildFromDom(DomModelData& root, rapidjson::Value& dom, DomModelContext* context, AZ::StackedString& path);
#if AZ_REFLECTION_PROTOTYPE_ENABLED
        static bool Describe(
            AZ::Reflection::IDescriber& describer, DomModelData& root, rapidjson::Value& dom, DomModelContext* context,
            const AZ::TypeId& targetType);
#endif
    };
} // namespace AzToolsFramework
