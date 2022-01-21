/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/OSAllocator.h>

// OS Allocations macros AZ_OS_MALLOC/AZ_OS_FREE
#include <AzCore/Memory/OSAllocator_Platform.h>

namespace AZ
{
    OSAllocator::pointer OSAllocator::allocate(size_type byteSize, align_type alignment)
    {
        return AZ_OS_MALLOC(byteSize, static_cast<AZStd::size_t>(alignment));
    }

    void OSAllocator::deallocate(pointer ptr, size_type, align_type)
    {
        AZ_OS_FREE(ptr);
    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type newAlignment)
    {
        return AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(newAlignment));
    }

    void OSAllocator::Merge([[maybe_unused]] IAllocator* aOther)
    {
        // Nothing to do, all allocations end up managed by the OS, no matter which instance
    }
}
