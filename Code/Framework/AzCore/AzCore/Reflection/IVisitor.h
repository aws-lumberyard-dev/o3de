/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once
#include <AzCore/Reflection/ReflectionConfig.h>
#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/base.h>
#include <AzCore/Reflection/Attributes.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::Reflection
{
    class IVisitor
    {
    public:
        AZ_RTTI(IVisitor, "{C33C8F7D-4676-4DA5-A90D-E4B38C2238DA}");

        public:
            ~IVisitor() = default;

            virtual void Visit([[maybe_unused]] AZStd::string& value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void Visit([[maybe_unused]] bool& value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void Visit([[maybe_unused]] int& value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectBegin([[maybe_unused]] const AZ::TypeId& typeId, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectEnd(){};
    };
} // namespace AZ::Reflection

#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
