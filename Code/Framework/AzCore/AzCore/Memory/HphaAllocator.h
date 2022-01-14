/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/AllocatorInterface.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZ
{
    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    * SubAllocator defines the allocator to be used underneath to do allocations
    */
    AZ_ALLOCATOR_VIRTUAL_INTERFACE(HphaAllocator, "{1ED481B0-53E2-4DCD-B016-4251D1A5AA8D}", OSAllocator);
}
