/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <limits>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::Debug
{
    class EventLogReader
    {
    public:
        bool ReadLog(const char* filePath);

        EventNameHash GetEventName() const;
        uint16_t GetEventSize() const;
        uint16_t GetEventFlags() const;
        uint64_t GetThreadId() const;

        AZStd::string_view GetString() const;
        template<typename T>
        const T* GetValue() const
        {
            AZ_Assert(sizeof(T) <= m_current->m_size, "Attempting to retrieve a value that's larger than the amount of stored data.");
            return reinterpret_cast<T*>(m_current + 1);
        }

        bool Next();

    private:
        void UpdateThreadId();

        AZStd::vector<uint8_t> m_buffer;
        IEventLogger::LogHeader m_logHeader;

        uint64_t m_currentThreadId{ 0 };
        IEventLogger::EventHeader* m_current{ nullptr };
    };


    class LocalFileEventLogger
        : public Interface<IEventLogger>::Registrar
    {
    public:
        inline static constexpr size_t MaxThreadCount = 512;

        ~LocalFileEventLogger() override;

        bool Start(const AZ::IO::Path& filePath, bool performanceMode = false) override;
        bool Start(AZStd::string_view outputPath, AZStd::string_view fileNameHint, bool performanceMode = false) override;
        void Stop() override;

        void Flush() override;

        void* RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags = 0) override;
        void RecordEventEnd() override;
        void RecordStringEvent(EventNameHash id, AZStd::string_view text, uint16_t flags = 0) override;

        AZStd::pair<ThreadData*, size_t> RecordPerformanceEventBegin(EventNameHash id, uint16_t size, uint16_t flags) override;
        void RecordPerformanceEventEnd(ThreadData* bufferData) override;

        bool IsPerformanceModeEnabled() override;
        void EnablePerformanceMode(bool enable) override;

    protected:

        struct ThreadStorage
        {
            ThreadStorage() = default;
            ~ThreadStorage();

            void Reset(LocalFileEventLogger* owner);

            AZStd::atomic<ThreadData*> m_data{ nullptr };
            ThreadData* m_pendingData{ nullptr };
            LocalFileEventLogger* m_owner{ nullptr };
        };

        void WriteCacheToDisk(ThreadData& threadData);

        ThreadStorage& GetThreadStorage();

        AZStd::fixed_vector<ThreadStorage*, MaxThreadCount> m_threadDataBlocks;

        AZStd::fixed_vector<ThreadData*, MaxThreadCount> m_deferredDataBlocks;

        AZ::IO::SystemFile m_file;
        AZStd::recursive_mutex m_fileGuard;
        AZStd::recursive_mutex m_fileWriteGuard;

        bool m_performanceMode { false };
        bool m_stopRequested { false };
    };
} // namespace AZ::Debug
