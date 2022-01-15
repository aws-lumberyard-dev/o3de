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
    AllocatorInterface* CreateHphaAllocatorPimpl(AllocatorInterface& mSubAllocator);
    void DestroyHphaAllocatorPimpl(AllocatorInterface& mSubAllocator, AllocatorInterface* allocator);

    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    * SubAllocator defines the allocator to be used underneath to do allocations
    */
    template<typename SubAllocatorType = OSAllocator>
    class HphaAllocator : public AllocatorInterface
    {
    public:
        AZ_TYPE_INFO(HphaAllocator, "{1ED481B0-53E2-4DCD-B016-4251D1A5AA8D}")

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

        void Merge(AllocatorInterface* aOther) override
        {
            m_allocatorPimpl->Merge(aOther);
        }

    private:
        AZ_DISABLE_COPY_MOVE(HphaAllocator)

        // Due the complexity of this allocator, we create a pimpl implementation
        AllocatorInterface* m_allocatorPimpl;
    };
}
