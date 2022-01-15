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
    AllocatorInterface* CreatePoolAllocatorPimpl(AllocatorInterface& mSubAllocator);
    void DestroyPoolAllocatorPimpl(AllocatorInterface& mSubAllocator, AllocatorInterface* allocator);

    /*!
     * Pool allocator
     * Specialized allocation for extremely fast small object memory allocations.
     * Pool Allocator is NOT thread safe, if you if need a thread safe version
     * use PoolAllocatorThreadSafe or do the sync yourself.
     */
    template<typename SubAllocatorType = OSAllocator>
    class PoolAllocator : public AllocatorInterface
    {
    public:
        AZ_TYPE_INFO(PoolAllocator, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}")

        PoolAllocator()
        {
            m_allocatorPimpl = CreatePoolAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get());
        }

        ~PoolAllocator() override
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

        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) override
        {
            return m_allocatorPimpl->reallocate(ptr, newSize, newAlignment);
        }

        void Merge(AllocatorInterface* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(PoolAllocator)

        // Due the complexity of this allocator, we create a pimpl implementation
        AllocatorInterface* m_allocatorPimpl;
    };

    AllocatorInterface* CreateThreadPoolAllocatorPimpl(AllocatorInterface& mSubAllocator);
    void DestroyThreadPoolAllocatorPimpl(AllocatorInterface& mSubAllocator, AllocatorInterface* allocator);

    /*!
     * Thread safe pool allocator. If you want to create your own thread pool heap,
     * inherit from ThreadPoolBase, as we need unique static variable for allocator type.
     */
    template<typename SubAllocatorType = OSAllocator>
    class ThreadPoolAllocator : public AllocatorInterface
    {
    public:
        AZ_TYPE_INFO(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}")

        ThreadPoolAllocator()
        {
            m_allocatorPimpl = CreateHphaAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get());
        }

        ~ThreadPoolAllocator() override
        {
            DestroyHphaAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get(), m_allocatorPimpl);
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

        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) override
        {
            return m_allocatorPimpl->reallocate(ptr, newSize, newAlignment);
        }

        void Merge(AllocatorInterface* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(ThreadPoolAllocator)

        // Due the complexity of this allocator, we create a pimpl implementation
        AllocatorInterface* m_allocatorPimpl;
    };
}
