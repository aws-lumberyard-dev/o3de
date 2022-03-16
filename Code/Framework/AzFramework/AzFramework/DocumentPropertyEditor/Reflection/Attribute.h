/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/functional.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/any.h>

namespace AZ::Reflection
{
    using AttributeDataType = AZStd::variant<bool, uint64_t, int64_t, double, AZStd::string_view, AZ::Uuid, AZStd::any>;

    struct IAttributes
    {
        using IterationCallback =
            AZStd::function<void(AZStd::string_view group, AZStd::string_view name, const AttributeDataType& attribute)>;

        virtual const AttributeDataType* Find(Name name) const = 0;
        virtual const AttributeDataType* Find(Name group, Name name) const = 0;
        virtual void ListAttributes(const IterationCallback& callback) const = 0;
    };
}
