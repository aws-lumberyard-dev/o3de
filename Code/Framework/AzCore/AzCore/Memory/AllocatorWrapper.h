/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/AllocatorInterface.h>

namespace AZ
{
    /**
     * Wrapper for a virtual interface of an allocator.
     *
     */
    class AllocatorWrapper : public AllocatorInterface
    {
        friend bool operator==(const AllocatorWrapper&, const AllocatorWrapper&);

    public:
        AllocatorWrapper(AllocatorInterface* allocator)
            : m_allocator(allocator)
        {
        }
        ~AllocatorWrapper() override = default;

        AllocatorWrapper(const AllocatorWrapper& aOther)
        {
            m_allocator = aOther.m_allocator;
        }

        pointer allocate(size_type byteSize, align_type alignment = 1) override
        {
            return m_allocator->allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, align_type = 0) override
        {
            m_allocator->deallocate(ptr, byteSize);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) override
        {
            return m_allocator->reallocate(ptr, newSize, newAlignment);
        }

        void Merge(AllocatorInterface* aOther) override
        {
            m_allocator->Merge(aOther);
        }

    private:
        AllocatorInterface* m_allocator;
    };

    bool operator==(const AllocatorWrapper& a, const AllocatorWrapper& b)
    {
        return a.m_allocator == b.m_allocator;
    }
}
