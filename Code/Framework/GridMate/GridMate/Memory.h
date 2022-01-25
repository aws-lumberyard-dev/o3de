/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_MEMORY_H
#define GM_MEMORY_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorWrappers.h>

namespace GridMate
{
    /**
    * GridMateAllocator is used by non-MP portions of GridMate
    */
   AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(GridMateAllocator, AZ::SystemAllocator, "{BB127E7A-E4EF-4480-8F17-0C10146D79E0}")

    /**
    * GridMateAllocatorMP is used by MP portions of GridMate
    */
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(GridMateAllocatorMP, AZ::SystemAllocator, "{FABCBC6E-B3E5-4200-861E-A3EC22592678}")

    //! GridMate system container allocator.
    using GridMateStdAlloc = GridMateAllocator;

    //! GridMate system container allocator.
    using SysContAlloc = GridMateAllocatorMP;
}   // namespace GridMate


#define GM_CLASS_ALLOCATOR(_type)       AZ_CLASS_ALLOCATOR(_type, GridMate::GridMateAllocatorMP, 0)
#define GM_CLASS_ALLOCATOR_DECL         AZ_CLASS_ALLOCATOR_DECL
#define GM_CLASS_ALLOCATOR_IMPL(_type)  AZ_CLASS_ALLOCATOR_IMPL(_type, GridMate::GridMateAllocatorMP, 0)

#endif // GM_MEMORY_H
