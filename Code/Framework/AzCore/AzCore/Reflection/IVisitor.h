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
        virtual ~IVisitor() = default;

        virtual void Visit([[maybe_unused]] bool& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void Visit([[maybe_unused]] int8_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] int16_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] int32_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] int64_t& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void Visit([[maybe_unused]] uint8_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] uint16_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] uint32_t& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] uint64_t& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void Visit([[maybe_unused]] float& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] double& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void Visit([[maybe_unused]] AZStd::string& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void VisitObjectBegin([[maybe_unused]] const AZ::TypeId& typeId, [[maybe_unused]] const IAttributes& attributes){};
        virtual void VisitObjectEnd(){};
    };
} // namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
