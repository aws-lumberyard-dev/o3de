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
        pointer ptr = AZ_OS_MALLOC(byteSize, static_cast<AZStd::size_t>(alignment));
#if defined(AZ_ENABLE_TRACING)
        // We assume 1 alignment because alignment is sometimes not passed in deallocate. This does mean that we are under-reporting
        // for cases where alignment != 1 and the OS could not find a block specifically for that alignment (the OS will give use a
        // block that is byteSize+(alignment-1) and place the ptr in the first address that satisfies the alignment). 
        const size_type allocatedSize = get_allocated_size(ptr, 1); 
        AddAllocated(allocatedSize);
        AddAllocationRecord(ptr, byteSize, allocatedSize, alignment, 1);
#endif
        return ptr;
    }

    void OSAllocator::deallocate(pointer ptr, size_type byteSize, [[maybe_unused]] align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = get_allocated_size(ptr, 1);
        RemoveAllocated(allocatedSize);
        RemoveAllocationRecord(ptr, byteSize, allocatedSize);
#endif
        AZ_OS_FREE(ptr);
    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        if (ptr)
        {
            const size_type previouslyAllocatedSize = get_allocated_size(ptr, 1);
            RemoveAllocated(previouslyAllocatedSize);
            RemoveAllocationRecord(ptr, 0, previouslyAllocatedSize);
        }
#endif
        // Realloc in most platforms doesnt support allocating from a nulltpr
        pointer newPtr = ptr ? AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(alignment))
                             : AZ_OS_MALLOC(newSize, static_cast<AZStd::size_t>(alignment));
#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = get_allocated_size(newPtr, 1);
        AddAllocated(allocatedSize);
        AddAllocationRecord(newPtr, newSize, allocatedSize, alignment, 1);
#endif
        return newPtr;
    }

    OSAllocator::size_type OSAllocator::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return ptr ? AZ_OS_MSIZE(ptr, alignment) : 0;
    }

    void OSAllocator::Merge([[maybe_unused]] IAllocator* aOther)
    {
        // Nothing to do regarding the allocations, we just need to move over all the tracking data
#if defined(AZ_ENABLE_TRACING)
        OSAllocator* other = azrtti_cast<OSAllocator*>(aOther);
        RecordingsMove(other);
#endif
    }
}
