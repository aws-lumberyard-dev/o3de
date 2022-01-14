/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZ
{
    /**
     * System allocator
     * The system allocator is the default allocator and also the allocator being used
     allocator for AZ memory lib, its also the default
     * allocator being usied in . It is a singleton (like all other allocators), but
     * must be initialized first and destroyed last. All other allocators
     * will use them for internal allocations. This doesn't mean all other allocators
     * will be sub allocators, because we might have different memory system on consoles.
     * But the allocator utility system will use the system allocator.
     */
    using SystemAllocator = HphaAllocator<OSAllocator>;
}
