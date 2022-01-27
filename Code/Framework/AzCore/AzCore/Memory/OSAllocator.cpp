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
        RecordAllocation(ptr, byteSize, alignment, byteSize); // we assume the OS doesnt produce fragmentation
#endif
        return ptr;
    }

    void OSAllocator::deallocate(pointer ptr, size_type byteSize, align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = AZ_OS_MSIZE(ptr, alignment);
        const size_type requestedSize = byteSize ? byteSize : allocatedSize; // if not passed, assume same as allocated
        RecordDeallocation(ptr, byteSize, alignment, allocatedSize);
#endif
        AZ_OS_FREE(ptr);
    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type newAlignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type previouslyRequestedSize = AZ_OS_MSIZE(ptr, newAlignment); // assume same alignment
        const size_type previouslyAllocatedSize = previouslyRequestedSize;
#endif
        pointer newPtr = AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(newAlignment));
#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = AZ_OS_MSIZE(newPtr, newAlignment);
        RecordReallocation(ptr, previouslyRequestedSize, newAlignment, previouslyAllocatedSize, newPtr, newSize, newAlignment, allocatedSize);
#endif
        return ptr;
    }

    OSAllocator::size_type OSAllocator::get_allocated_size(pointer ptr, align_type alignment) const
    {
        return AZ_OS_MSIZE(ptr, alignment);
    }

    void OSAllocator::Merge([[maybe_unused]] IAllocator* aOther)
    {
        // Nothing to do regarding the allocations, we just need to move over all the tracking data
#if defined(AZ_ENABLE_TRACING)
        OSAllocator* other = dynamic_cast<OSAllocator*>(aOther);
        RecordingsMove(other);
#endif
    }
}
