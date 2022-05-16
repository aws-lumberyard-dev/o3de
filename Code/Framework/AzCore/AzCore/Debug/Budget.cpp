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
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>

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

        BudgetImpl(AZStd::string_view name)
        {
            m_budgetNameLength = static_cast<uint8_t>(name.length());
            m_baseEventSize = m_budgetNameLength + sizeof(AZStd::chrono::system_clock::time_point) + sizeof(uint64_t);
            m_eventLogger = Interface<IEventLogger>::Get();
        }
        
        static constexpr size_t MaxProfileDepth = 32;        
        thread_local static AZStd::pair<IEventLogger::ThreadData*, size_t> s_eventData[MaxProfileDepth];
        thread_local static int8_t s_currentDepth;

        uint16_t m_budgetNameLength;
        uint16_t m_baseEventSize;
        IEventLogger* m_eventLogger;
    };

    thread_local AZStd::pair<IEventLogger::ThreadData*, size_t> BudgetImpl::s_eventData[BudgetImpl::MaxProfileDepth];
    thread_local int8_t BudgetImpl::s_currentDepth = -1;

    Budget::Budget(const char* name)
        : Budget( name, Crc32(name) )
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
        , m_budgetNameHash { name }
    {
        m_impl = aznew BudgetImpl(name);
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

    void Budget::BeginProfileRegion(AZStd::string_view event)
    {
        if (m_impl->m_eventLogger->IsPerformanceModeEnabled())
        {
            BudgetImpl::s_currentDepth++;
            if (BudgetImpl::s_currentDepth < BudgetImpl::MaxProfileDepth)
            {
                uint16_t eventSize = static_cast<uint16_t>(event.length());
                uint16_t bufferSize = m_impl->m_baseEventSize + eventSize;
                uint16_t flags = BudgetImpl::s_currentDepth;
                flags |= (m_impl->m_budgetNameLength << 8);
                auto perfEvent = m_impl->m_eventLogger->RecordPerformanceEventBegin(m_budgetNameHash, bufferSize, flags);

                if (perfEvent.first != nullptr)
                {
                    BudgetImpl::s_eventData[BudgetImpl::s_currentDepth] = perfEvent;
                    char* buffer = BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].first->m_buffer + BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].second;

                    memcpy(buffer, m_name, m_impl->m_budgetNameLength);

                    buffer += m_impl->m_budgetNameLength;
                    BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].second += m_impl->m_budgetNameLength;
                    memcpy(buffer, event.data(), eventSize);

                    buffer += eventSize;
                    BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].second += eventSize;
                    AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::high_resolution_clock::now();
                    memcpy(buffer, &startTime, sizeof(AZStd::chrono::system_clock::time_point));
                }
            }
            else
            {
                BudgetImpl::s_currentDepth--;
                AZ_Assert(false, "Max profile capture depth is %d", BudgetImpl::MaxProfileDepth);
                return;
            }
        }
    }

    void Budget::EndProfileRegion()
    {
        if (m_impl->m_eventLogger->IsPerformanceModeEnabled() && BudgetImpl::s_currentDepth >= 0)
        {
            char *buffer = BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].first->m_buffer + BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].second;
            
            AZStd::chrono::system_clock::time_point* startTime = reinterpret_cast<AZStd::chrono::system_clock::time_point*>(buffer);
            AZStd::chrono::system_clock::time_point endTime = AZStd::chrono::high_resolution_clock::now();
            uint64_t duration = (endTime - *startTime).count();
            
            buffer += sizeof(AZStd::chrono::system_clock::time_point);
            memcpy(buffer, &duration, sizeof(uint64_t));

            m_impl->m_eventLogger->RecordPerformanceEventEnd(BudgetImpl::s_eventData[BudgetImpl::s_currentDepth].first);
            BudgetImpl::s_currentDepth--;
        }
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }
} // namespace AZ::Debug
