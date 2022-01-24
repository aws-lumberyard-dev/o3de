/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    class IAllocator;

    /**
     * Global allocation manager. It has access to all created allocators IAllocator interface. And control
     * some global allocations.
     */
    class AllocatorManager
    {
    public:
        AllocatorManager() = default;
        ~AllocatorManager() = default;

        static AllocatorManager& Instance();

        AZ_FORCE_INLINE size_t GetNumAllocators() const
        {
            return m_numAllocators;
        }
        AZ_FORCE_INLINE IAllocator* GetAllocator(size_t index)
        {
            AZ_Assert(index < m_numAllocators, "Invalid allocator index %d [0,%d]!", index, m_numAllocators - 1);
            return m_allocators[index];
        }

        // Called from IAllocator
        void RegisterAllocator(IAllocator* alloc);
        void UnRegisterAllocator(IAllocator* alloc);

    private:
        AZ_DISABLE_COPY_MOVE(AllocatorManager)

        static constexpr size_t s_maxNumAllocators = 100;
        AZStd::array<IAllocator*, s_maxNumAllocators> m_allocators;
        size_t m_numAllocators = 0;
        AZStd::mutex m_allocatorListMutex;

        static AllocatorManager g_allocMgr; ///< The single instance of the allocator manager
    };
} // namespace AZ
