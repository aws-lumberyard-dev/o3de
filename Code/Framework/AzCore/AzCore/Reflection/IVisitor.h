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
    namespace Visitor
    {
        struct IStringAccess
        {
            virtual const AZ::TypeId& GetStringType() const = 0;
            virtual bool Set(AZStd::string_view) = 0;
        };

        struct IObjectAccess
        {
            virtual const AZ::TypeId& GetType() const = 0;
            virtual AZStd::string_view GetTypeName() const = 0;

            virtual void* Get() = 0;
            virtual const void* Get() const = 0;
        };

        class IRead
        {
        public:
            virtual ~IRead() = default;

            virtual void Visit(bool, const IAttributes& attributes){};
            virtual void Visit(int8_t value, const IAttributes& attributes){};
            virtual void VisitObjectBegin(const IObjectAccess& access, const IAttributes& attributes){};
            virtual void VisitObjectEnd(){};
            virtual void Visit(const AZStd::string_view value, const IStringAccess& acess, const IAttributes& attributes){};
        };

        class IReadWrite
        {
        public:
            virtual ~IReadWrite() = default;

            virtual void Visit(bool& value, const IAttributes& attributes){};
            virtual void Visit(int8_t& value, const IAttributes& attributes){};
            virtual void VisitObjectBegin(IObjectAccess& access, const IAttributes& attributes){};
            virtual void VisitObjectEnd(){};
            virtual void Visit(const AZStd::string_view value, IStringAccess& access, const IAttributes& attributes){};
        };
    };
}
#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
