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

            virtual void Visit([[maybe_unused]] bool value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void Visit([[maybe_unused]] int8_t value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectBegin([[maybe_unused]] const AZ::TypeId& typeId, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectEnd(){};
            virtual void Visit(
                [[maybe_unused]] const AZStd::string_view value,
                [[maybe_unused]] const IStringAccess& acess,
                [[maybe_unused]] const IAttributes& attributes){};
        };

        class IReadWrite
        {
        public:
            virtual ~IReadWrite() = default;

            virtual void Visit([[maybe_unused]] bool& value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void Visit([[maybe_unused]] int8_t& value, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectBegin([[maybe_unused]] const AZ::TypeId& typeId, [[maybe_unused]] const IAttributes& attributes){};
            virtual void VisitObjectEnd(){};
            virtual void Visit([[maybe_unused]] const AZStd::string_view value, [[maybe_unused]] const IAttributes& attributes){};
        };

        void VisitObjectBegin(const AZ::TypeId& typeId, const IAttributes& attributes)
        {
            IRead read;
            read.VisitObjectBegin(typeId, attributes);
        }

        void Visit(bool& value, const IAttributes& attributes)
        {
            IRead read;
            read.Visit(value, attributes);
        }

        void Visit(int8_t value, const IAttributes& attributes)
        {
            IRead read;
            read.Visit(value, attributes);
        }

        void Visit(const AZStd::string_view value, const IAttributes& attributes)
        {
            IReadWrite read;
            read.Visit(value, attributes);
        }

        void VisitObjectEnd()
        {
            return;
        }
    };
}
#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
