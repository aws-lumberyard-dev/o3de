/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorTrackingRecorder.h>

#if defined(AZ_ENABLE_TRACING)

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Memory/AllocatorDebug.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/spin_mutex.h>

// Amount of stack trace entries to record (amount of functions to record)
#define STACK_TRACE_DEPTH_RECORDING 20

#endif

namespace AZ
{
#if defined(AZ_ENABLE_TRACING)
    class AllocationRecord
    {
    public:
        AllocationRecord(void* address, AZStd::size_t requestedSize, AZStd::size_t allocatedSize, AZStd::size_t alignmentSize)
            : m_address(address)
            , m_requestedSize(requestedSize)
            , m_allocatedSize(allocatedSize)
            , m_alignmentSize(alignmentSize)
        {
        }

        static AllocationRecord Create(
            void* address,
            AZStd::size_t requestedSize,
            AZStd::size_t allocatedSize,
            AZStd::size_t alignmentSize,
            const AZStd::size_t stackFramesToSkip);

        void Print() const;

        bool operator<(const AllocationRecord& other) const
        {
            return this->m_address < other.m_address;
        }

        bool operator==(const AllocationRecord& other) const
        {
            return this->m_address == other.m_address;
        }

        void* m_address;
        AZStd::size_t m_requestedSize;
        AZStd::size_t m_allocatedSize;
        AZStd::size_t m_alignmentSize;
        using StackTrace = AZStd::vector<AZ::Debug::StackFrame, AZ::Debug::DebugAllocator>;
        StackTrace m_allocationStackTrace;
    };

    struct IAllocatorTrackingRecorderData 
    {
        AZStd::atomic_size_t m_requestedSize; // Total amount of requested bytes to the allocator
        AZStd::atomic_size_t m_allocatedSize; // Total amount of bytes allocated (i.e. requested to the OS)

        AZStd::spin_mutex m_allocationRecordsMutex;
        using AllocationRecords = AZStd::set<AllocationRecord, AZStd::less<AllocationRecord>, AZ::Debug::DebugAllocator>;
        AllocationRecords m_allocationRecords;
    };

    AllocationRecord AllocationRecord::Create(void* address, AZStd::size_t requestedSize, AZStd::size_t allocatedSize, AZStd::size_t alignmentSize, const AZStd::size_t stackFramesToSkip)
    {
        AllocationRecord record(address, requestedSize, allocatedSize, alignmentSize);
        record.m_allocationStackTrace.resize(STACK_TRACE_DEPTH_RECORDING); // make enough room for the recording
        unsigned int numOfFrames = Debug::StackRecorder::Record(record.m_allocationStackTrace.begin(), STACK_TRACE_DEPTH_RECORDING, static_cast<unsigned int>(stackFramesToSkip + 1));
        if (numOfFrames < STACK_TRACE_DEPTH_RECORDING)
        {
            record.m_allocationStackTrace.resize(numOfFrames);
            record.m_allocationStackTrace.shrink_to_fit();
        }
        return AZStd::move(record);
    }

    void AllocationRecord::Print() const
    {
        AZ_Printf("Memory", "Allocation Addr: 0%p, Requested Size: %zu, Allocated Size: %zu, Alignment: %zu\n", m_address, m_requestedSize, m_allocatedSize, m_alignmentSize);

        // We wont have more entries than STACK_TRACE_DEPTH_RECORDING
        Debug::SymbolStorage::StackLine lines[STACK_TRACE_DEPTH_RECORDING];
        const size_t stackSize = m_allocationStackTrace.size();
        Debug::SymbolStorage::DecodeFrames(m_allocationStackTrace.begin(), static_cast<unsigned int>(stackSize), lines);
        
        for (AZStd::size_t i = 0; i < stackSize; ++i)
        {
            AZ_Printf("Memory", "\t%s\n", lines[i]);
        }
    }

    void IAllocatorWithTracking::AddRequestedSize(AZStd::size_t requestedSize)
    {
        m_data->m_requestedSize += requestedSize;
    }

    void IAllocatorWithTracking::RemoveRequestedSize(AZStd::size_t requestedSize)
    {
        m_data->m_requestedSize -= requestedSize;
    }

    void IAllocatorWithTracking::AddAllocatedSize(AZStd::size_t allocatedSize)
    {
        m_data->m_allocatedSize += allocatedSize;
    }

    void IAllocatorWithTracking::RemoveAllocatedSize(AZStd::size_t allocatedSize)
    {
        m_data->m_allocatedSize -= allocatedSize;
    }

    void IAllocatorWithTracking::AddAllocationRecord(void* address, AZStd::size_t requestedSize, AZStd::size_t allocatedSize, AZStd::size_t alignmentSize, AZStd::size_t stackFramesToSkip)
    {
        AllocationRecord record = AllocationRecord::Create(address, requestedSize, allocatedSize, alignmentSize, stackFramesToSkip + 1);
        AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
        auto it = m_data->m_allocationRecords.emplace(AZStd::move(record));
        if (!it.second)
        {
            AZ_Assert(false, "Allocation with address 0%p already exists");
            AZ_Printf("Memory", "Trying to replace this record:");
            it.first->Print();
            AZ_Printf("Memory", "with this one:");
            record.Print();
        }
    }

    void IAllocatorWithTracking::RemoveAllocationRecord(void* address, [[maybe_unused]] AZStd::size_t requestedSize, [[maybe_unused]] AZStd::size_t allocatedSize)
    {
        AllocationRecord record(address, 0, 0, 0); // those other values do not matter because the set just compares the address
        AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
        auto it = m_data->m_allocationRecords.find(record);
        AZ_Assert(it != m_data->m_allocationRecords.end(), "Allocation with address 0%p was not found");
        AZ_Assert(it->m_requestedSize == requestedSize, "Mismatch on requested size")
        AZ_Assert(it->m_allocatedSize == allocatedSize, "Mismatch on allocated size")
        m_data->m_allocationRecords.erase(it);
    }

#endif // defined(AZ_ENABLE_TRACING)

    IAllocatorWithTracking::IAllocatorWithTracking()
    {
#if defined(AZ_ENABLE_TRACING)
        m_data = new (AZ::Debug::DebugAllocator().allocate(sizeof(IAllocatorTrackingRecorderData), alignof(IAllocatorTrackingRecorderData))) IAllocatorTrackingRecorderData();
#endif()
    }

    IAllocatorWithTracking::~IAllocatorWithTracking()
    {
#if defined(AZ_ENABLE_TRACING)
        m_data->~IAllocatorTrackingRecorderData();
        AZ::Debug::DebugAllocator().deallocate(m_data, sizeof(IAllocatorTrackingRecorderData), alignof(IAllocatorTrackingRecorderData));
#endif
    }

    void IAllocatorWithTracking::RecordingsMove([[maybe_unused]] IAllocatorTrackingRecorder* aOther)
    {
#if defined(AZ_ENABLE_TRACING)
        IAllocatorWithTracking* other = azrtti_cast<IAllocatorWithTracking*>(aOther);
        AZ_Assert(other, "Unexpected conversion, RecordingsMove should be reimplmented if IAllocatorTrackingRecorder is being used");
        m_data->m_requestedSize += other->m_data->m_requestedSize;
        m_data->m_allocatedSize += other->m_data->m_allocatedSize;
        other->m_data->m_requestedSize = 0;
        other->m_data->m_allocatedSize = 0;

        AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
        AZStd::scoped_lock lockOther(other->m_data->m_allocationRecordsMutex);
        m_data->m_allocationRecords.insert(other->m_data->m_allocationRecords.begin(), other->m_data->m_allocationRecords.end());
        other->m_data->m_allocationRecords.clear();
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetRequestedSize() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_requestedSize;
#else
        return 0;
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetAllocatedSize() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_allocatedSize;
#else
        return 0;
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetFragmentedSize() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_allocatedSize - m_data->m_requestedSize;
#else
        return 0;
#endif
    }

    void IAllocatorWithTracking::PrintAllocations() const
    {
#if defined(AZ_ENABLE_TRACING)
        // Create a copy of allocation records so other allocations can happen while we print these
        IAllocatorTrackingRecorderData::AllocationRecords allocations;
        {
            AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
            allocations = m_data->m_allocationRecords;
        }

        if (allocations.empty())
        {
            AZ_Printf("Memory", "There are no allocations in allocator 0%p (%s)\n", this, GetName());
        }
        else
        {
            AZ_Printf("Memory", "There are %d allocations in allocator 0%p (%s):\n", allocations.size(), this, GetName());
            for (const AllocationRecord& record : allocations)
            {
                record.Print();
            }
        }
#else
        AZ_Printf("Memory", "Allocation recording is disabled, AZ_ENABLE_TRACING needs to be enabled.");
#endif
    }

} // namespace AZ

#if defined(AZ_ENABLE_TRACING)
#undef STACK_TRACE_DEPTH_RECORDING
#endif
