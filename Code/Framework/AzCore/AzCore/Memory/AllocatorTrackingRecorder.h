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

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
#if defined(AZ_ENABLE_TRACING)
    class AllocationRecord;
    struct IAllocatorTrackingRecorderData;
#endif

    class IAllocatorTrackingRecorder
    {
    public:
        AZ_RTTI(IAllocatorTrackingRecorder, "{10468A58-A4E3-4FD0-8121-60F6DD13981C}")

    protected:
        virtual void RecordingsMove(IAllocatorTrackingRecorder* aOther) = 0;

    public:
        // query APIs

        // Total amount of requested bytes to the allocator
        virtual AZStd::size_t GetRequestedSize() const = 0;

        // Total amount of bytes allocated (i.e. requested to the OS)
        virtual AZStd::size_t GetAllocatedSize() const = 0;

        // Amount of memory not being used by the allocator (allocated but not assigned to any request)
        // Note: the value returned by this method may not be exact, there is the potential for a race condition
        //       between the substraction and assignment of other methods.
        virtual AZStd::size_t GetFragmentedSize() const = 0;

        // Prints all allocations that have been recorded
        virtual void PrintAllocations() const = 0;

        // Gets the amount of allocations (used in some tests)
        virtual AZStd::size_t GetAllocationCount() const = 0;
    };

    /// Default implementation of IAllocatorTrackingRecorder.
    /// We also inherit from IAllocator to prevent derived classes from having to inherit a dreaded diamond
    class IAllocatorWithTracking
        : public IAllocator
        , public IAllocatorTrackingRecorder
    {
    public:
        AZ_RTTI(IAllocatorWithTracking, "{FACD0B30-2983-46CB-8D48-EFB0E0637510}", IAllocator, IAllocatorTrackingRecorder)

        IAllocatorWithTracking();
        ~IAllocatorWithTracking();

        void RecordingsMove(IAllocatorTrackingRecorder* aOther) override;

        // query APIs

        // Total amount of requested bytes to the allocator
        AZStd::size_t GetRequestedSize() const override;

        // Total amount of bytes allocated (i.e. requested to the OS)
        AZStd::size_t GetAllocatedSize() const override;

        // Amount of memory not being used by the allocator (allocated but not assigned to any request)
        // Note: the value returned by this method may not be exact, there is the potential for a race condition
        //       between the substraction and assignment of other methods.
        AZStd::size_t GetFragmentedSize() const override;

        void PrintAllocations() const override;

        AZStd::size_t GetAllocationCount() const override;

        // Kept for backwards-compatibility reasons
        /////////////////////////////////////////////
        AZ_DEPRECATED_MESSAGE("Use GetAllocatedSize instead")
        size_type NumAllocatedBytes() const override
        {
            return GetAllocatedSize();
        }
        /////////////////////////////////////////////

    protected:
#if defined(AZ_ENABLE_TRACING)
        void AddAllocatedSize(AZStd::size_t allocatedSize);
        void RemoveAllocatedSize(AZStd::size_t allocatedSize);

        void AddAllocationRecord(void* address, AZStd::size_t requestedSize, AZStd::size_t allocatedSize, AZStd::size_t alignmentSize, AZStd::size_t stackFramesToSkip = 0);
        void RemoveAllocationRecord(void* address, AZStd::size_t requestedSize, AZStd::size_t allocatedSize);

        // we need to keep this header clean of includes since this will be included by allocators
        // we can end up with include loops where we cannot find definitions
        IAllocatorTrackingRecorderData* m_data;
#endif
    };
}
