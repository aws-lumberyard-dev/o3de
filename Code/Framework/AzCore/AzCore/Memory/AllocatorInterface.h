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

// Common functions that we maintain across every allocator, similar to the std::allocator interface
#define AZ_ALLOCATOR_INTERFACE(AllocatorName, ...)                                                                                         \
    using value_type = void;                                                                                                               \
    using pointer = void*;                                                                                                                 \
    using size_type = AZStd::size_t;                                                                                                       \
    using difference_type = AZStd::ptrdiff_t;                                                                                              \
    using align_type = AZStd::align_val_t;                                                                                                 \
    using propagate_on_container_move_assignment = AZStd::true_type;                                                                       \
                                                                                                                                           \
    AllocatorName(__VA_ARGS__);                                                                                                            \
    ~AllocatorName();                                                                                                                      \
                                                                                                                                           \
    pointer allocate(size_type byteSize, align_type alignment = align_type(1));                                                            \
    void deallocate(pointer ptr, size_type byteSize);                                                                                      \
    pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = align_type(1));                                           \
                                                                                                                                           \
    void Merge(AllocatorName* aOther);                                                                                                     \
                                                                                                                                           \
    AZ_DISABLE_COPY_MOVE(AllocatorName)

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
        using align_type = AZStd::align_val_t;
        using propagate_on_container_move_assignment = AZStd::true_type;

        virtual ~AllocatorInterface() = default;

        virtual pointer allocate(size_type byteSize, align_type alignment = align_type(1)) = 0;
        virtual void deallocate(pointer ptr, size_type byteSize) = 0;
        virtual pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = align_type(1)) = 0;

        virtual void Merge(AllocatorInterface* aOther) = 0;

        // Kept for backwards-compatibility reasons
        pointer_type Allocate(
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
        virtual void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) = 0;
        /// Resize an allocated memory block. Returns the new adjusted size (as close as possible or equal to the requested one) or 0 (if
        /// you don't support resize at all).
        virtual size_type Resize(pointer_type ptr, size_type newSize) = 0;
        /// Realloc an allocate memory memory block. Similar to Resize except it will move the memory block if needed. Return NULL if
        /// realloc is not supported or run out of memory.
        virtual pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) = 0;
        ///
        /// Returns allocation size for given address. 0 if the address doesn't belong to the allocator.
        virtual size_type AllocationSize(pointer_type ptr) = 0;
        /**
         * Call from the system when we want allocators to free unused memory.
         * IMPORTANT: This function is/should be thread safe. We can call it from any context to free memory.
         * Freeing the actual memory is optional (if you can), thread safety is a must.
         */
        virtual void GarbageCollect()
        {
        }

        virtual size_type NumAllocatedBytes() const = 0;
        /// Returns the capacity of the Allocator in bytes. If the return value is 0 the Capacity is undefined (usually depends on another
        /// allocator)
        virtual size_type Capacity() const = 0;
        /// Returns max allocation size if possible. If not returned value is 0
        virtual size_type GetMaxAllocationSize() const
        {
            return 0;
        }
        /// Returns the maximum contiguous allocation size of a single allocation
        virtual size_type GetMaxContiguousAllocationSize() const
        {
            return 0;
        }
        /**
         * Returns memory allocated by the allocator and available to the user for allocations.
         * IMPORTANT: this is not the overhead memory this is just the memory that is allocated, but not used. Example: the pool allocators
         * allocate in chunks. So we might be using one elements in that chunk and the rest is free/unallocated. This is the memory
         * that will be reported.
         */
        virtual size_type GetUnAllocatedMemory(bool isPrint = false) const
        {
            (void)isPrint;
            return 0;
        }
    };

    using IAllocator = AllocatorInterface;

    template<typename Allocator>
    class AllocatorInterfaceImpl : public AllocatorInterface
    {
    public:
        ~AllocatorInterfaceImpl() override = default;

        pointer allocate(size_type byteSize, align_type alignment = align_type(1)) override
        {
            return AZ::AllocatorInstance<Allocator>::Get().allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize) override
        {
            AZ::AllocatorInstance<Allocator>::Get().deallocate(ptr, byteSize);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = align_type(1)) override
        {
            return AZ::AllocatorInstance<Allocator>::Get().reallocate(ptr, newSize, newAlignment);
        }

        void Merge(AllocatorInterface* aOther) override
        {
            AZ::AllocatorInstance<Allocator>::Get().Merge(dynamic_cast<Allocator*>(aOther));
        }
    };

} // namespace AZ

#define AZ_ALLOCATOR_VIRTUAL_INTERFACE(AllocatorName, UUIDValue, SubAllocatorName)                                                         \
    AZ::AllocatorInterface* Create##AllocatorName##Pimpl(AZ::AllocatorInterface& subAllocator);                                            \
    void Destroy##AllocatorName##Pimpl(AZ::AllocatorInterface& subAllocator, AZ::AllocatorInterface* allocator);                           \
    template<typename SubAllocator = SubAllocatorName>                                                                                     \
    class AllocatorName                                                                                                                    \
    {                                                                                                                                      \
    public:                                                                                                                                \
        AZ_TYPE_INFO(AllocatorName, UUIDValue)                                                                                             \
        using value_type = void;                                                                                                           \
        using pointer = void*;                                                                                                             \
        using size_type = AZStd::size_t;                                                                                                   \
        using difference_type = AZStd::ptrdiff_t;                                                                                          \
        using align_type = AZStd::align_val_t;                                                                                             \
        using propagate_on_container_move_assignment = AZStd::true_type;                                                                   \
                                                                                                                                           \
        AllocatorName()                                                                                                                    \
        {                                                                                                                                  \
            m_allocator = Create##AllocatorName##Pimpl(m_subAllocator);                                                                    \
        }                                                                                                                                  \
        ~AllocatorName()                                                                                                                   \
        {                                                                                                                                  \
            Destroy##AllocatorName##Pimpl(m_subAllocator, m_allocator);                                                                    \
            m_allocator = nullptr;                                                                                                         \
        }                                                                                                                                  \
        pointer allocate(size_type byteSize, align_type alignment = align_type(1))                                                         \
        {                                                                                                                                  \
            return m_allocator->allocate(byteSize, alignment);                                                                             \
        }                                                                                                                                  \
        void deallocate(pointer ptr, size_type byteSize)                                                                                   \
        {                                                                                                                                  \
            m_allocator->deallocate(ptr, byteSize);                                                                                        \
        }                                                                                                                                  \
        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = align_type(1))                                        \
        {                                                                                                                                  \
            return m_allocator->reallocate(ptr, newSize, newAlignment);                                                                    \
        }                                                                                                                                  \
                                                                                                                                           \
        void Merge(AllocatorName* aOther)                                                                                                  \
        {                                                                                                                                  \
            m_allocator->Merge(aOther->m_allocator);                                                                                       \
        }                                                                                                                                  \
    private:                                                                                                                               \
        AZ_DISABLE_COPY_MOVE(AllocatorName)                                                                                                \
                                                                                                                                           \
        AllocatorInterface* m_allocator;                                                                                                   \
        AllocatorInterfaceImpl<typename SubAllocator> m_subAllocator;                                                                      \
    }
