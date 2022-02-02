/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    IAllocator* CreatePoolAllocatorPimpl(IAllocator& subAllocator);
    void DestroyPoolAllocatorPimpl(IAllocator& subAllocator, IAllocator* allocator);

    /*!
     * Pool allocator
     * Specialized allocation for extremely fast small object memory allocations.
     * Pool Allocator is NOT thread safe, if you if need a thread safe version
     * use PoolAllocatorThreadSafe or do the sync yourself.
     */
    template<typename SubAllocatorType = OSAllocator>
    class PoolAllocatorType : public IAllocator
    {
    public:
        PoolAllocatorType()
        {
            m_allocatorPimpl = CreatePoolAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get());
        }

        ~PoolAllocatorType() override
        {
            DestroyPoolAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get(), m_allocatorPimpl);
            m_allocatorPimpl = nullptr;
        }

        pointer allocate(size_type byteSize, align_type alignment = 1) override
        {
            return m_allocatorPimpl->allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override
        {
            m_allocatorPimpl->deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override
        {
            return m_allocatorPimpl->reallocate(ptr, newSize, alignment);
        }

        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return m_allocatorPimpl->get_allocated_size(ptr, alignment);
        }

        void Merge(IAllocator* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(PoolAllocatorType)

        // Due the complexity of this allocator, we create a pimpl implementation
        IAllocator* m_allocatorPimpl;
    };

    AZ_TYPE_INFO_TEMPLATE(PoolAllocatorType, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}", AZ_TYPE_INFO_TYPENAME);

    using PoolAllocator = PoolAllocatorType<SystemAllocator>;

    IAllocator* CreateThreadPoolAllocatorPimpl(IAllocator& subAllocator);
    void DestroyThreadPoolAllocatorPimpl(IAllocator& subAllocator, IAllocator* allocator);

    /*!
     * Thread safe pool allocator. If you want to create your own thread pool heap,
     * inherit from ThreadPoolBase, as we need unique static variable for allocator type.
     */
    template<typename SubAllocatorType = OSAllocator>
    class ThreadPoolAllocatorType : public IAllocator
    {
    public:
        AZ_TYPE_INFO(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}")

        ThreadPoolAllocatorType()
        {
            m_allocatorPimpl = CreateThreadPoolAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get());
        }

        ~ThreadPoolAllocatorType() override
        {
            DestroyThreadPoolAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get(), m_allocatorPimpl);
            m_allocatorPimpl = nullptr;
        }

        pointer allocate(size_type byteSize, align_type alignment = 1) override
        {
            return m_allocatorPimpl->allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override
        {
            m_allocatorPimpl->deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override
        {
            return m_allocatorPimpl->reallocate(ptr, newSize, alignment);
        }

        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return m_allocatorPimpl->get_allocated_size(ptr, alignment);
        }

        void Merge(IAllocator* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(ThreadPoolAllocatorType)

        // Due the complexity of this allocator, we create a pimpl implementation
        IAllocator* m_allocatorPimpl;
    };

    AZ_TYPE_INFO_TEMPLATE(ThreadPoolAllocatorType, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}", AZ_TYPE_INFO_TYPENAME);

    using ThreadPoolAllocator = ThreadPoolAllocatorType<SystemAllocator>;
}
