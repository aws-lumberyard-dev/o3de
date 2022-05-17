/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Platform.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/LocalFileEventLogger.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Settings/SettingsRegistry.h>

#include <time.h>


namespace AZ::Debug
{
    static constexpr const char* RegistryKey_TimestampLogFiles = "/Amazon/AzCore/EventLogger/TimestampLogFiles";

    //
    // EventLogReader
    //

    bool EventLogReader::ReadLog(const char* filePath)
    {
        using namespace AZ::IO;

        if (SystemFile::Exists(filePath))
        {
            SystemFile::SizeType size = SystemFile::Length(filePath);
            if (size == 0)
            {
                return false;
            }
            m_buffer.resize_no_construct(size);
            uint8_t* buffer = m_buffer.data();

            SystemFile::SizeType readSize = SystemFile::Read(filePath, buffer, size);
            if (readSize == size)
            {
                memcpy(&m_logHeader, buffer, sizeof(m_logHeader));
                m_current = reinterpret_cast<IEventLogger::EventHeader*>(buffer + sizeof(m_logHeader));
                UpdateThreadId();
                return true;
            }
        }
        return false;
    }

    EventNameHash EventLogReader::GetEventName() const
    {
        return m_current->m_eventId;
    }

    uint16_t EventLogReader::GetEventSize() const
    {
        return m_current->m_size;
    }

    uint16_t EventLogReader::GetEventFlags() const
    {
        return m_current->m_flags;
    }

    uint64_t EventLogReader::GetThreadId() const
    {
        return m_currentThreadId;
    }

    AZStd::string_view EventLogReader::GetString() const
    {
        return AZStd::string_view(reinterpret_cast<char*>(m_current + 1), m_current->m_size);
    }

    bool EventLogReader::Next()
    {
        size_t increment = AZ_SIZE_ALIGN_UP(sizeof(IEventLogger::EventHeader) + m_current->m_size, EventBoundary);
        uint8_t* bufferPosition = reinterpret_cast<uint8_t*>(m_current) + increment;
        if (bufferPosition < m_buffer.end())
        {
            m_current = reinterpret_cast<IEventLogger::EventHeader*>(bufferPosition);
            UpdateThreadId();
            return true;
        }
        return false;
    }

    void EventLogReader::UpdateThreadId()
    {
        if (GetEventName() == PrologEventHash)
        {
            auto prolog = reinterpret_cast<IEventLogger::Prolog*>(m_current);
            m_currentThreadId = prolog->m_threadId;
        }
    }

    //
    // LocalFileEventLogger
    //

    LocalFileEventLogger::~LocalFileEventLogger()
    {
        if (m_file.IsOpen())
        {
            Stop();
        }

        // thread blocks should have already been flushed above, so
        // this is purely to clear the logger ownership safely
        while (!m_threadDataBlocks.empty())
        {
            m_threadDataBlocks.back()->Reset(nullptr);
        }
    }

    bool LocalFileEventLogger::Start(const AZ::IO::Path& filePath, bool performanceMode)
    {
        using namespace AZ::IO;

        AZStd::scoped_lock lock(m_fileGuard);
        if (m_file.Open(filePath.c_str(), SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_CREATE_PATH))
        {
            LogHeader defaultHeader;
            m_file.Write(&defaultHeader, sizeof(LogHeader));
            m_stopped = false;
            m_performanceMode = performanceMode;
            return true;
        }
        return false;
    }

    bool LocalFileEventLogger::Start(AZStd::string_view outputPath, AZStd::string_view fileNameHint)
    {
        using namespace AZ::IO;

        FixedMaxPath filePath{ outputPath };
        FixedMaxPathString fileName{ fileNameHint };

        SystemFile::CreateDir(filePath.c_str());

        bool includeTimestamp = false;
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        settingsRegistry->Get(includeTimestamp, RegistryKey_TimestampLogFiles);

        if (includeTimestamp)
        {
            time_t rawtime;
            time(&rawtime);

            tm timeinfo;
            azlocaltime(&rawtime, &timeinfo);

            constexpr int timestampSize = 64;
            char timestamp[timestampSize]{ 0 };

            // based the ISO-8601 standard (YYYY-MM-DDTHH-mm-ssTZD) e.g., 20210224_1122
            strftime(timestamp, timestampSize, "%Y%m%d_%H%M", &timeinfo);

            fileName = AZ::IO::FixedMaxPathString::format("%.*s_%s",
                aznumeric_cast<int>(fileNameHint.size()), fileNameHint.data(),
                timestamp);
        }

        filePath /= fileName;
        filePath.ReplaceExtension("azel");

        return Start(filePath.c_str());
    }

    bool LocalFileEventLogger::StartPerformanceCapture(const AZ::IO::Path& filePath)
    {
        AZ_TracePrintf("EventLogger", "Starting performance capture");

        if (m_file.IsOpen())
        {
            m_logFilePath = m_file.Name();
            Stop();
        }

        ClearAllThreadStorage();

        if (!Start(filePath, true))
        {
            Start(m_logFilePath);
            AZ_Assert(false, "Failed to start performance capture!");
            return false;
        }

        return true;
    }

    void LocalFileEventLogger::StopPerformanceCapture()
    {
        if (m_performanceMode)
        {
            m_stopRequested = true;
        }
        else
        {
            Stop();
            ClearAllThreadStorage();
            Start(m_logFilePath);
            AZ_TracePrintf("EventLogger", "Stopped performance capture");
        }
    }

    void LocalFileEventLogger::Stop()
    {
        Flush();
        m_file.Close();
        m_stopped = true;
    }

    void LocalFileEventLogger::Flush()
    {
        // Create new storage for a thread to write to. This will replace the storage already on the thread
        // so it can continue to write and is not blocked during a flush. The data that was swapped in can
        // then again be used for the next thread.
        ThreadData* replacementData = new ThreadData();
        bool flushedThread[MaxThreadCount] = {};

        {
            AZStd::scoped_lock fileGuardLock(m_fileGuard);
            bool allFlushed;
            do
            {
                allFlushed = true;
                for (size_t i = 0; i < m_threadDataBlocks.size(); ++i)
                {
                    // Don't flush threads that have already been flushed because during high activity
                    // this can cause this loop to always find more threads to flush resulting in
                    // taking a long time to exit the Flush function. As a side effect of this it will
                    // decrease the time between retrying a thread it previous failed to claim which
                    // increases the chance it gets to switch the data.
                    if (flushedThread[i])
                    {
                        continue;
                    }

                    ThreadStorage* thread = m_threadDataBlocks[i];
                    ThreadData* threadData = thread->m_data;
                    if (!threadData)
                    {
                        allFlushed = false;
                        continue;
                    }

                    // ensure the thread ID propagates after the exchange
                    replacementData->m_threadId = threadData->m_threadId;
                    if (!thread->m_data.compare_exchange_strong(threadData, replacementData))
                    {
                        // Since no other flush can reach this point due to the lock, failing this means
                        // that between looking up the address for the data and the swap the owning thread
                        // has started a write, so bail for now and come back to this one at a later time
                        // to try again.
                        allFlushed = false;
                        continue;
                    }

                    {
                        AZStd::scoped_lock fileWriteGuardLock(m_fileWriteGuard);
                        WriteCacheToDisk(*threadData);
                    }
                    replacementData = threadData;
                    flushedThread[i] = true;
                }
            } while (!allFlushed);
            m_file.Flush();
        }

        delete replacementData;
    }

    void* LocalFileEventLogger::RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags)
    {
        if (m_stopped || m_performanceMode)
        {
            return nullptr;
        }

        ThreadStorage& threadStorage = GetThreadStorage();
        ThreadData* threadData = threadStorage.m_data;

        // Set to nullptr so other threads doing a flush can't pick this up.
        while (!threadStorage.m_data.compare_exchange_strong(threadData, nullptr))
        {
        }

        uint32_t writeSize = AZ_SIZE_ALIGN_UP(sizeof(EventHeader) + size, EventBoundary);
        if (threadData->m_usedBytes + writeSize >= ThreadData::BufferSize)
        {
            AZStd::scoped_lock lock(m_fileWriteGuard);
            WriteCacheToDisk(*threadData);
        }

        char* eventBuffer = (threadData->m_buffer + threadData->m_usedBytes);
        EventHeader* header = reinterpret_cast<EventHeader*>(eventBuffer);
        header->m_eventId = id;
        header->m_size = size;
        header->m_flags = flags;
        threadData->m_usedBytes += writeSize;

        // cache the event data so it doesn't get picked up by calls to flush
        // before it has been committed
        threadStorage.m_pendingData = threadData;

        return (eventBuffer + sizeof(EventHeader));
    }

    void LocalFileEventLogger::RecordEventEnd()
    {
        if (m_stopped || m_performanceMode)
        {
            return;
        }

        // swap the pending data to commit the event
        ThreadStorage& threadStorage = GetThreadStorage();
        ThreadData* expectedData = nullptr;
        while (!threadStorage.m_data.compare_exchange_strong(expectedData, threadStorage.m_pendingData))
        {
        }
        threadStorage.m_pendingData = nullptr;
    }

    void LocalFileEventLogger::RecordStringEvent(EventNameHash id, AZStd::string_view text, uint16_t flags)
    {
        if (m_stopped || m_performanceMode)
        {
            return;
        }

        constexpr size_t maxSize = AZStd::numeric_limits<decltype(EventHeader::m_size)>::max();
        const size_t stringLen = text.length();

        if (stringLen > maxSize)
        {
            AZ_Assert(false, "Failed to write event!  String too large to store with the event logger.");
            return;
        }

        void* eventText = RecordEventBegin(id, aznumeric_cast<uint16_t>(stringLen), flags);
        memcpy(eventText, text.data(), stringLen);
        RecordEventEnd();
    }

    void LocalFileEventLogger::RecordPerformanceEventBegin(EventNameHash id, uint16_t size, uint16_t flags, ThreadData*& outBuffer, size_t& outOffset)
    {
        outBuffer = nullptr;
        outOffset = 0;

        if (!m_performanceMode)
        {
            AZ_Assert(false, "Cannot record performance events when performance mode is disabled!");
            return;
        }
        
        ThreadStorage& threadStorage = GetThreadStorage();
        ThreadData* threadData = threadStorage.m_data;
        ThreadData* deferredThreadData = threadStorage.m_pendingData;
        
        if (m_stopRequested)
        {
            if (deferredThreadData && threadData->m_refCount == 0 && deferredThreadData->m_refCount == 0)
            {
                delete deferredThreadData;
                threadStorage.m_pendingData = nullptr;
                m_deferredDataCount--;

                if (m_deferredDataCount.load() == 0)
                {
                    m_stopRequested = false;
                    m_performanceMode = false;
                    StopPerformanceCapture();
                }
            }
            return;
        }

        uint32_t writeSize = AZ_SIZE_ALIGN_UP(sizeof(EventHeader) + size, EventBoundary);
        if (threadData->m_usedBytes + writeSize >= ThreadData::BufferSize)
        {
            if (threadData->m_refCount == 0)
            {
                AZStd::scoped_lock lock(m_fileWriteGuard);
                WriteCacheToDisk(*threadData);
            }
            else if (deferredThreadData->m_refCount == 0)
            {
                threadStorage.m_data = deferredThreadData;
                threadStorage.m_pendingData = threadData;
                threadData = threadStorage.m_data;
            }
            else
            {
                AZ_Assert(false, "Not all references of deferred thread data have been returned! Deferred thread data cannot be held for longer than 1 frame!");
                return;
            }
        }

        size_t usedBytesBeforeEvent = threadData->m_usedBytes;
        char* eventBuffer = (threadData->m_buffer + usedBytesBeforeEvent);
        EventHeader* header = reinterpret_cast<EventHeader*>(eventBuffer);
        header->m_eventId = id;
        header->m_size = size;
        header->m_flags = flags;
        threadData->m_usedBytes += writeSize;
        threadData->m_refCount++;

        outBuffer = threadData;
        outOffset = usedBytesBeforeEvent + sizeof(EventHeader);
    }

    void LocalFileEventLogger::RecordPerformanceEventEnd(ThreadData* bufferData)
    {
        if (!m_performanceMode)
        {
            return;
        }

        bufferData->m_refCount--;

        ThreadStorage& threadStorage = GetThreadStorage();

        if (bufferData->m_refCount == 0 && (bufferData == threadStorage.m_pendingData || m_stopRequested))
        {                
            {
                AZStd::scoped_lock guard(m_fileWriteGuard);
                WriteCacheToDisk(*bufferData);
            }
        }
    }

    bool LocalFileEventLogger::IsPerformanceModeEnabled()
    {
        return m_performanceMode;
    }

    void LocalFileEventLogger::WriteCacheToDisk(ThreadData& threadData)
    {
        // ensure the front loaded prolog is accurate, ThreadData objects
        // are recycled during flush
        Prolog* prologHeader = reinterpret_cast<Prolog*>(threadData.m_buffer);
        prologHeader->m_eventId = PrologEventHash;
        prologHeader->m_size = sizeof(prologHeader->m_threadId);
        prologHeader->m_flags = 0; // unused in the prolog
        prologHeader->m_threadId = threadData.m_threadId;

        m_file.Write(threadData.m_buffer, threadData.m_usedBytes);
        threadData.m_usedBytes = sizeof(Prolog); // keep enough room for the next chunk's prolog
    }

    auto LocalFileEventLogger::GetThreadStorage()->ThreadStorage&
    {
        thread_local static ThreadStorage s_storage;
        s_storage.Reset(this);
        return s_storage;
    }

    void LocalFileEventLogger::ClearAllThreadStorage()
    {
        if (!m_stopped)
        {
            AZ_Assert(false, "Cannot reset thread local storage data when the EventLogger is still running!");
            return;
        }

        for (int i = 0; i < m_threadDataBlocks.size(); i++)
        {
            m_threadDataBlocks[i]->Reset(nullptr);
        }

        m_threadDataBlocks.clear();
    }

    //
    // LocalFileEventLogger::ThreadStorage
    //

    LocalFileEventLogger::ThreadStorage::~ThreadStorage()
    {
        Reset(nullptr);
    }

    void LocalFileEventLogger::ThreadStorage::Reset(LocalFileEventLogger* owner)
    {
        if (m_owner == owner)
        {
            return;
        }

        if (m_owner)
        {
            AZStd::scoped_lock guard(m_owner->m_fileGuard);

            // Save to access thread data because of the lock.
            ThreadData* data = m_data;
            if (data->m_usedBytes > 0)
            {
                AZ_Assert(data->m_refCount == 0, "Thread data pointer still being cached when thread is being destroyed!");
                m_owner->WriteCacheToDisk(*data);
            }

            auto it = AZStd::find(m_owner->m_threadDataBlocks.begin(), m_owner->m_threadDataBlocks.end(), this);
            if (it != m_owner->m_threadDataBlocks.end())
            {
                m_owner->m_threadDataBlocks.erase(it);
            }

            delete data;
        }

        m_owner = owner;

        if (m_owner)
        {
            // Deliberately using system memory instead of regular allocators. If debug allocators
            // are available in the future those should be used instead.
            ThreadData* data = new ThreadData();
            data->m_threadId = azlossy_caster(AZStd::hash<AZStd::thread_id>{}(AZStd::this_thread::get_id()));
            m_data = data;

            if (!m_owner->m_performanceMode)
            {
                AZStd::scoped_lock guard(m_owner->m_fileGuard);
                m_owner->m_threadDataBlocks.push_back(this);
            }
            else
            {
                ThreadData* deferredData = new ThreadData();
                deferredData->m_threadId = azlossy_caster(AZStd::hash<AZStd::thread_id>{}(AZStd::this_thread::get_id()));
                m_pendingData = deferredData;
                m_owner->m_deferredDataCount++;
            }
        }
    }
} // namespace AZ::Debug
