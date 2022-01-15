/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    void* OperatorNew(std::size_t size)
    {
        return AllocatorInstance<SystemAllocator>::Get().allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT);
    }

    void* OperatorNew(std::size_t size, std::size_t align)
    {
        return AllocatorInstance<SystemAllocator>::Get().allocate(size, align);
    }

    void OperatorDelete(void* ptr)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, 0);
    }

    void OperatorDelete(void* ptr, std::size_t size)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size);
    }

    void OperatorDelete(void* ptr, std::size_t size, std::size_t align)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size, align);
    }

    void* OperatorNewArray(std::size_t size)
    {
        return AllocatorInstance<SystemAllocator>::Get().allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT);
    }

    void* OperatorNewArray(std::size_t size, std::size_t align)
    {
        return AllocatorInstance<SystemAllocator>::Get().allocate(size, align);
    }

    void OperatorDeleteArray(void* ptr)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, 0);
    }

    void OperatorDeleteArray(void* ptr, std::size_t size)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size);
    }

    void OperatorDeleteArray(void* ptr, AZStd::size_t size, AZStd::size_t align)
    {
        AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size, align);
    }
}
