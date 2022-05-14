/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Budget.h"

#include <AzCore/Module/Environment.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>
#include <AzCore/std/containers/stack.h>

AZ_DEFINE_BUDGET(Animation);
AZ_DEFINE_BUDGET(Audio);
AZ_DEFINE_BUDGET(AzCore);
AZ_DEFINE_BUDGET(Editor);
AZ_DEFINE_BUDGET(Entity);
AZ_DEFINE_BUDGET(Game);
AZ_DEFINE_BUDGET(System);
AZ_DEFINE_BUDGET(Physics);

namespace AZ::Debug
{
    struct BudgetImpl
    {
        AZ_CLASS_ALLOCATOR(BudgetImpl, AZ::SystemAllocator, 0);
        // TODO: Budget implementation for tracking budget wall time per-core, memory, etc.

        BudgetImpl(Statistics::StatisticalProfilerProxy::StatisticalProfilerType& profiler)
            : m_profiler(profiler)
        {
        }

        Statistics::StatisticalProfilerProxy::StatisticalProfilerType& m_profiler;
        thread_local static AZStd::stack<AZ::Crc32> s_eventIds;
        thread_local static AZStd::stack<AZStd::chrono::system_clock::time_point> s_regionStartTimes;
    };

    thread_local AZStd::stack<AZ::Crc32> BudgetImpl::s_eventIds;
    thread_local AZStd::stack<AZStd::chrono::system_clock::time_point> BudgetImpl::s_regionStartTimes;

    Budget::Budget(const char* name)
        : Budget( name, Crc32(name) )
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
    {
        if (auto statsProfiler = Interface<Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
        {
            m_impl = aznew BudgetImpl(statsProfiler->GetProfiler(m_crc));
        }
    }

    Budget::~Budget()
    {
        if (m_impl)
        {
            delete m_impl;
        }
    }

    // TODO:Budgets Methods below are stubbed pending future work to both update budget data and visualize it

    void Budget::PerFrameReset()
    {
    }

    void Budget::BeginProfileRegion(AZ::Crc32 eventId)
    {
        if (m_impl && m_impl->m_profiler.IsEnabled())
        {
            BudgetImpl::s_regionStartTimes.push(AZStd::chrono::high_resolution_clock::now());
            BudgetImpl::s_eventIds.push(eventId);
        }
    }

    void Budget::EndProfileRegion()
    {
        if (m_impl && m_impl->m_profiler.IsEnabled() && !BudgetImpl::s_regionStartTimes.empty())
        {
            AZStd::chrono::system_clock::time_point endTime = AZStd::chrono::high_resolution_clock::now();
            AZStd::chrono::microseconds duration = endTime - BudgetImpl::s_regionStartTimes.top();
            m_impl->m_profiler.PushSample(BudgetImpl::s_eventIds.top(), static_cast<double>(duration.count()));
            BudgetImpl::s_regionStartTimes.pop();
            BudgetImpl::s_eventIds.pop();
        }
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }
} // namespace AZ::Debug
