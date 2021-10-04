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
        AZ_RTTI(IVisitor, "{F68FB00E-3F3E-426D-9A7E-D5D79658C82E}");
        virtual ~IVisitor() = default;
        virtual void Visit([[maybe_unused]] bool& value, [[maybe_unused]] const IAttributes& attributes){};
        virtual void Visit([[maybe_unused]] int8_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] int16_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] int32_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] int64_t& value, [[maybe_unused]] const IAttributes& attributes){};

        //virtual void Visit([[maybe_unused]] uint8_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] uint16_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] uint32_t& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] uint64_t& value, [[maybe_unused]] const IAttributes& attributes){};

        //virtual void Visit([[maybe_unused]] float& value, [[maybe_unused]] const IAttributes& attributes){};
        //virtual void Visit([[maybe_unused]] double& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void Visit([[maybe_unused]] AZStd::string& value, [[maybe_unused]] const IAttributes& attributes){};

        virtual void VisitObjectBegin([[maybe_unused]] const AZ::TypeId& typeId, [[maybe_unused]] const IAttributes& attributes){};
        virtual void VisitObjectEnd(){};
    };
} // namespace AZ::Reflection
#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
