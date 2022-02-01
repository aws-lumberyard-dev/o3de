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
        AddRequestedSize(byteSize);
        const size_type allocatedSize = get_allocated_size(ptr, alignment);
        AddAllocatedSize(allocatedSize);
        AddAllocationRecord(ptr, byteSize, allocatedSize, alignment, 1);
#endif
        return ptr;
    }

    void OSAllocator::deallocate(pointer ptr, size_type byteSize, align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type allocatedSize = get_allocated_size(ptr, alignment);
        const size_t requestedSize = byteSize ? byteSize : allocatedSize;
        RemoveRequestedSize(requestedSize); // if not passed, assume same as allocated
        RemoveAllocatedSize(allocatedSize);
        RemoveAllocationRecord(ptr, requestedSize, allocatedSize);
#endif
        AZ_OS_FREE(ptr);
    }

    OSAllocator::pointer OSAllocator::reallocate(pointer ptr, size_type newSize, align_type newAlignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type previouslyRequestedSize = get_allocated_size(ptr, newAlignment); // assume same alignment
        RemoveAllocatedSize(previouslyRequestedSize);
        RemoveRequestedSize(previouslyRequestedSize);
        RemoveAllocationRecord(ptr, previouslyRequestedSize, previouslyRequestedSize);
#endif
        // Realloc in most platforms doesnt support allocating from a nulltpr
        pointer newPtr = ptr ? AZ_OS_REALLOC(ptr, newSize, static_cast<AZStd::size_t>(newAlignment))
                             : AZ_OS_MALLOC(newSize, static_cast<AZStd::size_t>(newAlignment));
#if defined(AZ_ENABLE_TRACING)
        AddRequestedSize(newSize);
        const size_type allocatedSize = get_allocated_size(newPtr, newAlignment);
        AddRequestedSize(allocatedSize);
        AddAllocationRecord(ptr, newSize, allocatedSize, newAlignment, 1);
#endif
        return ptr;
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
