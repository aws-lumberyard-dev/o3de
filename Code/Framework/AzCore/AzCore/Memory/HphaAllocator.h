/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/AllocatorTrackingRecorder.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/memory/AllocatorInstance.h>

namespace AZ
{
    IAllocatorWithTracking* CreateHphaAllocatorPimpl(IAllocator& mSubAllocator);
    void DestroyHphaAllocatorPimpl(IAllocator& mSubAllocator, IAllocatorWithTracking* allocator);

    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    * SubAllocator defines the allocator to be used underneath to do allocations
    */
    template<typename SubAllocatorType = OSAllocator>
    class HphaAllocator : public IAllocator, public IAllocatorTrackingRecorder
    {
    public:
        AZ_RTTI(HphaAllocator, "{1ED481B0-53E2-4DCD-B016-4251D1A5AA8D}", IAllocator, IAllocatorTrackingRecorder)

        HphaAllocator()
        {
            m_allocatorPimpl = CreateHphaAllocatorPimpl(AZ::AllocatorInstance<SubAllocatorType>::Get());
        }

        ~HphaAllocator() override
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

        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return m_allocatorPimpl->get_allocated_size(ptr, alignment);
        }

        void Merge(IAllocator* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

        void GarbageCollect() override
        {
            m_allocatorPimpl->GarbageCollect();
        }

        // IAllocatorTrackingRecorder
        
        AZStd::size_t GetRequestedSize() const override
        {
            return m_allocatorPimpl->GetRequestedSize();
        }

        // Total amount of bytes allocated (i.e. requested to the OS)
        AZStd::size_t GetAllocatedSize() const override
        {
            return m_allocatorPimpl->GetAllocatedSize();
        }

        AZStd::size_t GetFragmentedSize() const override
        {
            return m_allocatorPimpl->GetFragmentedSize();
        }

        void PrintAllocations() const override
        {
            m_allocatorPimpl->PrintAllocations();
        }

    protected:
        void RecordingsMove(IAllocatorTrackingRecorder* aOther) override
        {
            m_allocatorPimpl->RecordingsMove(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(HphaAllocator)

        // Due the complexity of this allocator, we create a pimpl implementation
        IAllocatorWithTracking* m_allocatorPimpl;
    };
}
