/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/AllocatorTrackingRecorder.h>

namespace AZ
{
    /**
     * The OSAllocator is a wrapper to direct OS allocation, it simply wraps OS allocation into our
     * memory system framework
     */
    class OSAllocator : public IAllocatorWithTracking
    {
    public:
        AZ_TYPE_INFO(OSAllocator, "{9F835EE3-F23C-454E-B4E3-011E2F3C8118}")

        OSAllocator() = default;
        virtual ~OSAllocator() = default;

        pointer allocate(size_type byteSize, align_type alignment = 1) override;
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;
        pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        void Merge(IAllocator* aOther) override;

        AZ_DISABLE_COPY_MOVE(OSAllocator)
    };
}
