/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/AllocatorWrappers.h>

namespace AZStd
{
    class stateless_allocator : public AZ::AllocatorGlobalWrapper<AZ::OSAllocator>
    {
    public:
        AZ_TYPE_INFO(stateless_allocator, "{E4976C53-0B20-4F39-8D41-0A76F59A7D68}");

        stateless_allocator() = default;
        stateless_allocator(const stateless_allocator& rhs) = default;
        stateless_allocator& operator=(const stateless_allocator& rhs) = default;

        bool is_lock_free();
        bool is_stale_read_allowed();
        bool is_delayed_recycling();
    };

    bool operator==(const stateless_allocator& left, const stateless_allocator& right);
    bool operator!=(const stateless_allocator& left, const stateless_allocator& right);
}
