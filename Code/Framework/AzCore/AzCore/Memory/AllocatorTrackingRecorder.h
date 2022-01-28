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

#if defined(AZ_ENABLE_TRACING)
#include <AzCore/std/parallel/atomic.h>
#endif

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class IAllocatorTrackingRecorder
    {
    protected:
        AZ_TYPE_INFO(IAllocatorTrackingRecorder, "{10468A58-A4E3-4FD0-8121-60F6DD13981C}")

        // recording APIs
        virtual void RecordAllocation(
            void* ptr, AZStd::size_t requestedSize, AZStd::size_t requestedAlignment, AZStd::size_t allocatedSize) = 0;

        virtual void RecordDeallocation(
            void* ptr, AZStd::size_t requestedSize, AZStd::size_t requestedAlignment, AZStd::size_t allocatedSize) = 0;

        virtual void RecordReallocation(
            void* previousPtr,
            AZStd::size_t previousRequestedSize,
            AZStd::size_t previousRequestedAlignment,
            AZStd::size_t previousAllocatedSize,
            void* newPtr,
            AZStd::size_t newRequestedSize,
            AZStd::size_t newRequestedAlignment,
            AZStd::size_t newAllocatedSize) = 0;

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
    };

    /// Default implementation of IAllocatorTrackingRecorder.
    /// We also inherit from IAllocator to prevent derived classes from having to inherit a dreaded diamond
    class IAllocatorWithTracking
        : public IAllocator
        , public IAllocatorTrackingRecorder
    {
    public:
        AZ_TYPE_INFO(IAllocatorWithTracking, "{FACD0B30-2983-46CB-8D48-EFB0E0637510}")

        IAllocatorWithTracking()
            : m_requestedSize(0)
            , m_allocatedSize(0)
        {
        }

        // recording APIs
        void RecordAllocation(
            [[maybe_unused]] void* ptr,
            [[maybe_unused]] AZStd::size_t requestedSize,
            [[maybe_unused]] AZStd::size_t requestedAlignment,
            [[maybe_unused]] AZStd::size_t allocatedSize) override
        {
#if defined(AZ_ENABLE_TRACING)
            if (ptr) // otherwise the allocation didnt happen
            {
                m_requestedSize += requestedSize;
                m_allocatedSize += allocatedSize;
            }
#endif
        }

        void RecordDeallocation(
            [[maybe_unused]] void* ptr,
            [[maybe_unused]] AZStd::size_t requestedSize,
            [[maybe_unused]] AZStd::size_t requestedAlignment,
            [[maybe_unused]] AZStd::size_t allocatedSize) override
        {
#if defined(AZ_ENABLE_TRACING)
            m_requestedSize -= requestedSize;
            m_allocatedSize -= allocatedSize;
#endif
        }

        void RecordReallocation(
            [[maybe_unused]] void* previousPtr,
            [[maybe_unused]] AZStd::size_t previousRequestedSize,
            [[maybe_unused]] AZStd::size_t previousRequestedAlignment,
            [[maybe_unused]] AZStd::size_t previousAllocatedSize,
            [[maybe_unused]] void* newPtr,
            [[maybe_unused]] AZStd::size_t newRequestedSize,
            [[maybe_unused]] AZStd::size_t newRequestedAlignment,
            [[maybe_unused]] AZStd::size_t newAllocatedSize) override
        {
#if defined(AZ_ENABLE_TRACING)
            if (newPtr) // otherwise the reallocation didnt happen
            {
                m_requestedSize -= previousRequestedSize;
                m_requestedSize += newRequestedSize;
                m_allocatedSize -= previousAllocatedSize;
                m_allocatedSize += newAllocatedSize;
            }
#endif
        }

        void RecordingsMove(IAllocatorTrackingRecorder* aOther) override
        {
            IAllocatorWithTracking* other = azrtti_cast<IAllocatorWithTracking*>(aOther);
            AZ_Assert(other, "Unexpected conversion, RecordingsMove should be reimplmented if IAllocatorTrackingRecorder is being used");
            m_requestedSize += other->m_requestedSize;
            m_allocatedSize += other->m_allocatedSize;
            other->m_requestedSize = 0;
            other->m_allocatedSize = 0;
        }

        // query APIs

        // Total amount of requested bytes to the allocator
        AZStd::size_t GetRequestedSize() const override
        {
#if defined(AZ_ENABLE_TRACING)
            return m_requestedSize;
#else
            return 0;
#endif
        }

        // Total amount of bytes allocated (i.e. requested to the OS)
        AZStd::size_t GetAllocatedSize() const override
        {
#if defined(AZ_ENABLE_TRACING)
            return m_allocatedSize;
#else
            return 0;
#endif
        }

        // Amount of memory not being used by the allocator (allocated but not assigned to any request)
        // Note: the value returned by this method may not be exact, there is the potential for a race condition
        //       between the substraction and assignment of other methods.
        AZStd::size_t GetFragmentedSize() const override
        {
#if defined(AZ_ENABLE_TRACING)
            return m_allocatedSize - m_requestedSize;
#else
            return 0;
#endif
        }

        // Kept for backwards-compatibility reasons
        /////////////////////////////////////////////
        AZ_DEPRECATED_MESSAGE("Use GetAllocatedSize instead")
        size_type NumAllocatedBytes() const override
        {
            return GetAllocatedSize();
        }

    protected:
#if defined(AZ_ENABLE_TRACING)
        AZStd::atomic_size_t m_requestedSize; // Total amount of requested bytes to the allocator
        AZStd::atomic_size_t m_allocatedSize; // Total amount of bytes allocated (i.e. requested to the OS)
#endif
    };
}
