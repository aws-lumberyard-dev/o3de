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
#include <AzCore/Reflection/IDescriptor.h>
#include <AzCore/Reflection/IVisitor.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/typetraits.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ::Reflection
{
    class ReflectionRegistrationContext : public AZ::ReflectContext
    {
    public:
        class Description
        {
        public:
            virtual AZStd::string_view GetName() const = 0;
            virtual const AZ::IRttiHelper* GetRtti() const = 0;
            virtual AZStd::any CreateInstance() const = 0;

            virtual void Describe(IDescriber& describer) const = 0;
            virtual void Visit(const void*, Visitor::IRead& visitor) const = 0;
            virtual void Visit(void*, Visitor::IReadWrite& visitor) const = 0;
        };

        using ListCallback = AZStd::function<bool(const Description*)>;

        template<typename T>
        void Register();

        virtual const Description* Find(const AZ::TypeId& typeId) const = 0;
        virtual const Description* Find(AZStd::string_view typeName) const = 0;
        virtual void ListDescriptions(ListCallback& callback) const = 0;
    };
}//namespace AZ::Reflection
#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
