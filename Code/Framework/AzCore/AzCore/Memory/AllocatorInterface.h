/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/alignment_of.h>

#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZ
{
    /**
     * Allocator interface base class used by AllocatorInterfaceImpl to wrap an allocator in a virtual
     * interface to be able to not depend on the implementation. Useful for allocators to be able to configure
     * sub allocators through templates but avoid having the implementation of the class in the header.
     */
    class AllocatorInterface
    {
    public:
        using value_type = void;
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using align_type = AZStd::size_t;
        using propagate_on_container_move_assignment = AZStd::true_type;

        AllocatorInterface() = default;
        AllocatorInterface(AllocatorInterface&) {}
        virtual ~AllocatorInterface() = default;

        virtual pointer allocate(size_type byteSize, align_type alignment = 1) = 0;
        virtual void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) = 0;
        virtual pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) = 0;

        virtual void Merge(AllocatorInterface* aOther) = 0;

        // Convenient functions
        template<typename TType>
        pointer allocate()
        {
            return allocate(sizeof(TType), static_cast<align_type>(AZStd::alignment_of<TType>::value));
        }

        template<typename TType>
        void deallocate(pointer ptr)
        {
            return deallocate(ptr, sizeof(TType), static_cast<align_type>(AZStd::alignment_of<TType>::value));
        }

        AZ_DISABLE_COPY_MOVE(AllocatorInterface)

        // Kept for backwards-compatibility reasons
        /////////////////////////////////////////////
        pointer Allocate(
            size_type byteSize,
            size_type alignment = 1,
            [[maybe_unused]] int flags = 0,
            [[maybe_unused]] const char* name = nullptr,
            [[maybe_unused]] const char* fileName = nullptr,
            [[maybe_unused]] int lineNum = 0,
            [[maybe_unused]] unsigned int suppressStackRecord = 0)
        {
            return allocate(byteSize, static_cast<align_type>(alignment));
        }

        void DeAllocate(pointer ptr, size_type byteSize = 0, [[maybe_unused]] size_type alignment = 0)
        {
            deallocate(ptr, byteSize);
        }

        /// Resize an allocated memory block. Returns the new adjusted size (as close as possible or equal to the requested one) or 0 (if
        /// you don't support resize at all).
        size_type Resize([[maybe_unused]] pointer ptr, [[maybe_unused]] size_type newSize)
        {
            return 0;
        }

        /// Realloc an allocate memory memory block. Similar to Resize except it will move the memory block if needed. Return NULL if
        /// realloc is not supported or run out of memory.
        pointer ReAllocate(pointer ptr, size_type newSize, size_type newAlignment)
        {
            return reallocate(ptr, newSize, static_cast<align_type>(newAlignment));
        }

        ///
        /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
        size_type AllocationSize([[maybe_unused]] pointer ptr)
        {
            return 0;
        }

        /**
         * Call from the system when we want allocators to free unused memory.
         * IMPORTANT: This function is/should be thread safe. We can call it from any context to free memory.
         * Freeing the actual memory is optional (if you can), thread safety is a must.
         */
        void GarbageCollect()
        {
        }

        size_type NumAllocatedBytes() const
        {
            return 0;
        }

        /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another
        /// allocator)
        size_type Capacity() const
        {
            return 0;
        }

        /// Returns max allocation size if possible. If not returned value is 0
        size_type GetMaxAllocationSize() const
        {
            return 0;
        }

        /// Returns the maximum contiguous allocation size of a single allocation
        size_type GetMaxContiguousAllocationSize() const
        {
            return 0;
        }
        /**
         * Returns memory allocated by the allocator and available to the user for allocations.
         * IMPORTANT: this is not the overhead memory this is just the memory that is allocated, but not used. Example: the pool allocators
         * allocate in chunks. So we might be using one elements in that chunk and the rest is free/unallocated. This is the memory
         * that will be reported.
         */
        size_type GetUnAllocatedMemory([[maybe_unused]] bool isPrint = false) const
        {
            return 0;
        }
        /////////////////////////////////////////////
    };

    using IAllocator = AllocatorInterface;

} // namespace AZ


#define DECLARE_AZ_ALLOCATOR_WITH_SUBALLOCATOR(AllocatorName, UUIDValue, ...)                                                              \
    class AllocatorName : public AllocatorInterface                                                                                        \
    {                                                                                                                                      \
        friend class AllocatorInstance;                                                                                                    \
                                                                                                                                           \
    public:                                                                                                                                \
        AZ_TYPE_INFO(AllocatorName, UUIDValue)                                                                                             \
                                                                                                                                           \
        pointer allocate(size_type byteSize, align_type alignment = 1) override;                                                           \
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;                                           \
        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) override;                                          \
                                                                                                                                           \
        void Merge(AllocatorInterface* aOther) override;                                                                                   \
                                                                                                                                           \
    private:                                                                                                                               \
        AllocatorName();                                                                                                                   \
        ~AllocatorName();                                                                                                                  \
        AZ_DISABLE_COPY_MOVE(AllocatorName)                                                                                                \
                                                                                                                                           \
        AllocatorInterface* Create();                                                                                                      \
        void Destroy(AllocatorInterface* allocator);                                                                                       \
                                                                                                                                           \
        SubAllocator m_subAllocator;                                                                                                       \
    }
