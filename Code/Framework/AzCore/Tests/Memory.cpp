/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzTest/TestTypes.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#include <AzCore/std/containers/intrusive_slist.h>
#include <AzCore/std/containers/intrusive_list.h>

#include <AzCore/std/chrono/clocks.h>

#include <AzCore/std/functional.h>



using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    class MemoryTrackingFixture
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
        }
        void TearDown() override
        {
        }
    };

    class SystemAllocatorTest
        : public MemoryTrackingFixture
    {
        static const int            m_threadStackSize = 128 * 1024;
        static const unsigned int   m_maxNumThreads = 10;
        AZStd::thread_desc          m_desc[m_maxNumThreads];
        AZ::SystemAllocator*        m_sysAlloc;

    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_sysAlloc = new AZ::SystemAllocator();

            for (unsigned int i = 0; i < m_maxNumThreads; ++i)
            {
                // Set the threads stack size
                m_desc[i].m_stackSize = m_threadStackSize;

                // Allocate stacks for the platforms that can't do it themself.
            }
        }

        void TearDown() override
        {
            delete m_sysAlloc;
            m_sysAlloc = nullptr;

            // Free allocated stacks.
            MemoryTrackingFixture::TearDown();
        }

        void ThreadFunc()
        {
#ifdef _DEBUG
            static const int numAllocations = 100;
#else
            static const int numAllocations = 10000;
#endif
            void* addresses[numAllocations] = {nullptr};

            //////////////////////////////////////////////////////////////////////////
            // Allocate
            AZStd::size_t totalAllocSize = 0;
            for (int i = 0; i < numAllocations; ++i)
            {
                AZStd::size_t size = AZStd::GetMax(rand() % 256, 1);
                // supply all debug info, so we don't need to record the stack.
                addresses[i] = m_sysAlloc->allocate(size, 8);
                memset(addresses[i], 1, size);
                totalAllocSize += size;
            }
            //////////////////////////////////////////////////////////////////////////

            EXPECT_GE(m_sysAlloc->GetRequested(), totalAllocSize);

            //////////////////////////////////////////////////////////////////////////
            // Deallocate
            for (int i = numAllocations-1; i >=0; --i)
            {
                m_sysAlloc->deallocate(addresses[i]);
            }
            //////////////////////////////////////////////////////////////////////////
        }

        void run()
        {
            void* address[100];
            {
                SystemAllocator sysAllocator;

                for (int i = 0; i < 100; ++i)
                {
                    address[i] = m_sysAlloc->allocate(1000, 32);
                    EXPECT_NE(nullptr, address[i]);
                    EXPECT_EQ(0, ((size_t)address[i] & 31)); // check alignment
                    EXPECT_GE(m_sysAlloc->get_allocated_size(address[i]), 1000); // check allocation size
                }

                EXPECT_GE(m_sysAlloc->GetRequested(), 100000); // we requested 100 * 1000 so we should have at least this much allocated

                for (int i = 0; i < 100; ++i)
                {
                    m_sysAlloc->deallocate(address[i]);
                }

                ////////////////////////////////////////////////////////////////////////
                // Create some threads and simulate concurrent allocation and deallocation
                AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
                {
                    AZStd::thread m_threads[m_maxNumThreads];
                    for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                    {
                        m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&SystemAllocatorTest::ThreadFunc, this));
                        // give some time offset to the threads so we can test alloc and dealloc at the same time.
                        //AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(500));
                    }

                    for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                    {
                        m_threads[i].join();
                    }
                }
                //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
                //AZ_Printf("UnitTest::SystemAllocatorTest::deafult","Time: %d Ms\n",exTime.count());
                //////////////////////////////////////////////////////////////////////////
            }

            memset(address, 0, AZ_ARRAY_SIZE(address) * sizeof(void*));

            SystemAllocator sysAllocator;

            for (int i = 0; i < 100; ++i)
            {
                address[i] = m_sysAlloc->allocate(1000, 32);
                EXPECT_NE(nullptr, address[i]);
                EXPECT_EQ(0, ((size_t)address[i] & 31)); // check alignment
                EXPECT_GE(m_sysAlloc->get_allocated_size(address[i]), 1000); // check allocation size
            }

            EXPECT_GE(m_sysAlloc->GetRequested(), 100000); // we requested 100 * 1000 so we should have at least this much allocated

// If tracking and recording is enabled, we can verify that the alloc info is valid
#if defined(AZ_ENABLE_TRACING)
            EXPECT_EQ(m_sysAlloc->GetAllocationCount(), 100);
            const AllocationRecordVector& records = m_sysAlloc->GetAllocationRecords();
            EXPECT_EQ(100, records.size());
            for (const AllocationRecord& record : records)
            {
                EXPECT_EQ(32, record.m_alignment);
                EXPECT_GE(record.m_requested, 1000); // since we are requesting alignment, GE since this allocation will go to the tree and it stores a 8-byte header

                bool found = false;
                int foundIndex = 0; // After finding it for the first time, save the index so it can be reused

#   if defined(AZ_PLATFORM_WINDOWS)
                if (!record.m_allocationStackTrace.empty())
                {
                    // For windows we should be able to decode the program counters into readable content.
                    // This is possible on deprecated platforms too, but we would need to load the map file manually and so on... it's
                    // tricky. Note: depending on where the tests are run from the call stack may differ.
                    SymbolStorage::StackLine stackLine[20];
                    SymbolStorage::DecodeFrames(&record.m_allocationStackTrace[0], AZ_ARRAY_SIZE(stackLine), stackLine);

                    for (int idx = foundIndex; idx < AZ_ARRAY_SIZE(stackLine); idx++)
                    {
                        if (strstr(stackLine[idx], "SystemAllocatorTest::run"))
                        {
                            found = true;
                            break;
                        }
                        else
                        {
                            foundIndex++;
                        }
                    }
                }
                EXPECT_TRUE(found);
#   endif // defined(AZ_PLATFORM_WINDOWS)
            }
#endif //#if defined(AZ_DEBUG_BUILD)

            // Free all memory
            for (int i = 0; i < 100; ++i)
            {
                m_sysAlloc->deallocate(address[i]);
            }

            m_sysAlloc->GarbageCollect();
            EXPECT_LT(m_sysAlloc->GetRequested(), 1024);

            //////////////////////////////////////////////////////////////////////////
            // realloc test
            address[0] = nullptr;
            static const unsigned int checkValue = 0x0badbabe;
            // create tree (non pool) allocation (we usually pool < 256 bytes)
            address[0] = m_sysAlloc->allocate(2048, 16);
            *(unsigned*)(address[0]) = checkValue; // set check value
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 2048, 16);
            address[0] = m_sysAlloc->ReAllocate(address[0], 1024, 16); // test tree big -> tree small
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 1024, 16);
            address[0] = m_sysAlloc->ReAllocate(address[0], 4096, 16); // test tree small -> tree big
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 4096, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 128, 16); // test tree -> pool,
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 128, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 64, 16); // pool big -> pool small
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 64, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 64, 16); // pool sanity check
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 64, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 192, 16); // pool small -> pool big
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 192, 16);
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 2048, 16); // pool -> tree
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 2048, 16);
            ;
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            address[0] = m_sysAlloc->ReAllocate(address[0], 2048, 16); // tree sanity check
            AZ_TEST_ASSERT_CLOSE(m_sysAlloc->get_allocated_size(address[0]), 2048, 16);
            ;
            EXPECT_EQ(checkValue, *(unsigned*)address[0]);
            m_sysAlloc->deallocate(address[0], 2048, 16);
            // TODO realloc with different alignment tests
            //////////////////////////////////////////////////////////////////////////

            // run some thread allocations.
            //////////////////////////////////////////////////////////////////////////
            // Create some threads and simulate concurrent allocation and deallocation
            //AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
            {
                AZStd::thread m_threads[m_maxNumThreads];
                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&SystemAllocatorTest::ThreadFunc, this));
                    // give some time offset to the threads so we can test alloc and dealloc at the same time.
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(500));
                }

                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
            //AZ_Printf("UnitTest::SystemAllocatorTest","Time: %d Ms\n",exTime.count());
            //////////////////////////////////////////////////////////////////////////
        }
    };

    TEST_F(SystemAllocatorTest, Test)
    {
        run();
    }

    class PoolAllocatorTest
        : public MemoryTrackingFixture
    {
    protected:
        bool m_isDynamic;
        int m_numStaticPages;
        PoolAllocator* m_poolAllocator;
    public:
        PoolAllocatorTest(bool isDynamic = true, int numStaticPages = 0)
            : m_isDynamic(isDynamic)
            , m_numStaticPages(numStaticPages)
            , m_poolAllocator(nullptr)
        {
        }

        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_poolAllocator = new PoolAllocator();
        }

        void TearDown() override
        {
            delete m_poolAllocator;
            m_poolAllocator = nullptr;

            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            // 64 should be the max number of different pool sizes we can allocate.
            void* address[64];
            //////////////////////////////////////////////////////////////////////////
            // Allocate different pool sizes
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));

            // try any size from 8 to 256 (which are supported pool sizes)
            int i = 0;
            for (int size = 8; size <= 256; ++i, size += 8)
            {
                address[i] = m_poolAllocator->allocate(size, 8);
                EXPECT_GE(m_poolAllocator->get_allocated_size(address[i]), (AZStd::size_t)size);
                memset(address[i], 1, size);
            }

            EXPECT_GE(m_poolAllocator->GetRequested(), 0);
            EXPECT_EQ(32, m_poolAllocator->GetAllocationCount());
            
            for (i = 0; address[i] != nullptr; ++i)
            {
                m_poolAllocator->deallocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            m_poolAllocator->GarbageCollect();
            EXPECT_EQ(0, m_poolAllocator->GetRequested());
            EXPECT_EQ(0, m_poolAllocator->GetAllocationCount());
            
            //////////////////////////////////////////////////////////////////////////
            // Allocate many elements from the same size
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));
            for (unsigned int j = 0; j < AZ_ARRAY_SIZE(address); ++j)
            {
                address[j] = m_poolAllocator->allocate(256, 8);
                EXPECT_GE(m_poolAllocator->get_allocated_size(address[j]), 256);
                memset(address[j], 1, 256);
            }

            EXPECT_GE(m_poolAllocator->GetRequested(), AZ_ARRAY_SIZE(address) * 256);
            EXPECT_EQ(AZ_ARRAY_SIZE(address), m_poolAllocator->GetAllocationCount());
 
            for (unsigned int j = 0; j < AZ_ARRAY_SIZE(address); ++j)
            {
                m_poolAllocator->deallocate(address[j]);
            }
            //////////////////////////////////////////////////////////////////////////

            m_poolAllocator->GarbageCollect();
            EXPECT_EQ(0, m_poolAllocator->GetRequested());
            EXPECT_EQ(0, m_poolAllocator->GetAllocationCount());
        }
    };

    TEST_F(PoolAllocatorTest, Test)
    {
        run();
    }

    class PoolAllocatorDynamicWithStaticPagesTest
        : public PoolAllocatorTest
    {
    public:
        PoolAllocatorDynamicWithStaticPagesTest()
            : PoolAllocatorTest(true, 10) {}                                      // just create 10 pages we will allocate more than
    };

    TEST_F(PoolAllocatorDynamicWithStaticPagesTest, Test)
    {
        run();
    }

    class PoolAllocatorStaticPagesTest
        : public PoolAllocatorTest
    {
    public:
        PoolAllocatorStaticPagesTest()
            : PoolAllocatorTest(false, 50) {}
    };

    TEST_F(PoolAllocatorStaticPagesTest, Test)
    {
        run();
    }

    /**
     * Tests ThreadPoolAllocator
     */
    class ThreadPoolAllocatorTest
        : public MemoryTrackingFixture
    {
        static const int            m_threadStackSize = 128 * 1024;
        static const unsigned int   m_maxNumThreads = 10;
        AZStd::thread_desc          m_desc[m_maxNumThreads];

        //\todo lock free
        struct AllocClass
            : public AZStd::intrusive_slist_node<AllocClass>
        {
            int m_data;
        };
        typedef AZStd::intrusive_slist<AllocClass, AZStd::slist_base_hook<AllocClass> > SharedAllocStackType;
        AZStd::mutex    m_mutex;

        SharedAllocStackType    m_sharedAlloc;
#ifdef _DEBUG
        static const int        m_numSharedAlloc = 100; ///< Number of shared alloc free.
#else
        static const int        m_numSharedAlloc = 10000; ///< Number of shared alloc free.
#endif
#if (ATOMIC_ADDRESS_LOCK_FREE==2)  // or we can use locked atomics
        AZStd::atomic_bool      m_doneSharedAlloc;
#else
        volatile bool           m_doneSharedAlloc;
#endif

        bool m_isDynamic;
        int m_numStaticPages;
        ThreadPoolAllocator* m_poolAllocator;

    public:
        ThreadPoolAllocatorTest(bool isDynamic = true, int numStaticPages = 0)
            : m_isDynamic(isDynamic)
            , m_numStaticPages(numStaticPages)
            , m_poolAllocator(nullptr)
        {
        }

        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();

            m_doneSharedAlloc = false;
#if defined(AZ_PLATFORM_WINDOWS)
            SetCriticalSectionSpinCount(m_mutex.native_handle(), 4000);
#endif

            m_poolAllocator = new ThreadPoolAllocator();

            for (unsigned int i = 0; i < m_maxNumThreads; ++i)
            {
                m_desc[i].m_stackSize = m_threadStackSize;
            }
        }

        void TearDown() override
        {
            delete m_poolAllocator;
            m_poolAllocator = nullptr;

            MemoryTrackingFixture::TearDown();
        }

        void AllocDeallocFunc()
        {
#ifdef _DEBUG
            static const int numAllocations = 100;
#else
            static const int numAllocations = 10000;
#endif
            void* addresses[numAllocations] = {nullptr};

            //////////////////////////////////////////////////////////////////////////
            // Allocate
            for (int i = 0; i < numAllocations; ++i)
            {
                AZStd::size_t size = AZStd::GetMax(1, ((i + 1) * 2) % 256);
                addresses[i] = m_poolAllocator->allocate(size, 8);
                EXPECT_NE(addresses[i], nullptr);
                memset(addresses[i], 1, size);
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Deallocate
            for (int i = numAllocations-1; i >=0; --i)
            {
                m_poolAllocator->deallocate(addresses[i]);
            }
            //////////////////////////////////////////////////////////////////////////
        }

        /**
         * Function that does allocations and pushes them on a lock free stack
         */
        void SharedAlloc()
        {
            for (int i = 0; i < m_numSharedAlloc; ++i)
            {
                AZStd::size_t minSize = sizeof(AllocClass);
                AZStd::size_t size = AZStd::GetMax((AZStd::size_t)(rand() % 256), minSize);
                AllocClass* ac = reinterpret_cast<AllocClass*>(m_poolAllocator->allocate(size, alignof(AllocClass)));
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_sharedAlloc.push_back(*ac);
            }
        }

        /**
        * Function that does deallocations from the lock free stack
        */
        void SharedDeAlloc()
        {
            AllocClass* ac;
            int isDone = 0;
            while (isDone!=2)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                while (!m_sharedAlloc.empty())
                {
                    ac = &m_sharedAlloc.front();
                    m_sharedAlloc.pop_front();
                    m_poolAllocator->deallocate(ac);
                }

                if (m_doneSharedAlloc) // once we know we don't add more elements, make one last check and exit.
                {
                    ++isDone;
                }
            }
        }

        class MyThreadPoolAllocator : public PoolAllocator
        {
        public:
            AZ_CLASS_ALLOCATOR(MyThreadPoolAllocator, SystemAllocator, 0);
            AZ_TYPE_INFO(MyThreadPoolAllocator, "{28D80F96-19B1-4465-8278-B53989C44CF1}");
        };

        void run()
        {
            // 64 should be the max number of different pool sizes we can allocate.
            void* address[64];

            m_poolAllocator->GarbageCollect();

            //////////////////////////////////////////////////////////////////////////
            // Allocate different pool sizes
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));

            // try any size from 8 to 256 (which are supported pool sizes)
            int j = 0;
            for (int size = 8; size <= 256; ++j, size += 8)
            {
                address[j] = m_poolAllocator->allocate(size, 8);
                EXPECT_GE(m_poolAllocator->get_allocated_size(address[j]), (AZStd::size_t)size);
                memset(address[j], 1, size);
            }

            EXPECT_GE(m_poolAllocator->GetRequested(), 4126);
            EXPECT_EQ(32, m_poolAllocator->GetAllocationCount());

            for (int i = 0; address[i] != nullptr; ++i)
            {
                m_poolAllocator->deallocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            m_poolAllocator->GarbageCollect();
            EXPECT_EQ(0, m_poolAllocator->GetRequested());
            EXPECT_EQ(0, m_poolAllocator->GetAllocationCount());
            
            //////////////////////////////////////////////////////////////////////////
            // Allocate many elements from the same size
            memset(address, 0, AZ_ARRAY_SIZE(address)*sizeof(void*));
            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(address); ++i)
            {
                address[i] = m_poolAllocator->allocate(256, 8);
                EXPECT_GE(m_poolAllocator->get_allocated_size(address[i]), 256);
                memset(address[i], 1, 256);
            }

            EXPECT_GE(m_poolAllocator->GetRequested(), AZ_ARRAY_SIZE(address) * 256);
            EXPECT_EQ(AZ_ARRAY_SIZE(address), m_poolAllocator->GetAllocationCount());

            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(address); ++i)
            {
                m_poolAllocator->deallocate(address[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            m_poolAllocator->GarbageCollect();
            EXPECT_EQ(0, m_poolAllocator->GetRequested());
            EXPECT_EQ(0, m_poolAllocator->GetAllocationCount());

            //////////////////////////////////////////////////////////////////////////
            // Create some threads and simulate concurrent allocation and deallocation
            //AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();
            {
                AZStd::thread m_threads[m_maxNumThreads];
                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::AllocDeallocFunc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //////////////////////////////////////////////////////////////////////////
            //AZStd::chrono::microseconds exTime = AZStd::chrono::system_clock::now() - startTime;
            //AZ_Printf("UnitTest","Time: %d Ms\n",exTime.count());

            //////////////////////////////////////////////////////////////////////////
            // Spawn some threads that allocate and some that deallocate together
            {
                AZStd::thread m_threads[m_maxNumThreads];

                for (unsigned int i = m_maxNumThreads/2; i <m_maxNumThreads; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::SharedDeAlloc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads/2; ++i)
                {
                    m_threads[i] = AZStd::thread(m_desc[i], AZStd::bind(&ThreadPoolAllocatorTest::SharedAlloc, this));
                }

                for (unsigned int i = 0; i < m_maxNumThreads/2; ++i)
                {
                    m_threads[i].join();
                }

                m_doneSharedAlloc = true;

                for (unsigned int i = m_maxNumThreads/2; i <m_maxNumThreads; ++i)
                {
                    m_threads[i].join();
                }
            }
            //////////////////////////////////////////////////////////////////////////

            // Our pools will support only 1024 byte allocations
            MyThreadPoolAllocator myAllocator;

            void* pooled128 = myAllocator.allocate(128, 128);
            myAllocator.deallocate(pooled128);

            AZ_TEST_START_TRACE_SUPPRESSION;
            [[maybe_unused]] void* pooled2048 = myAllocator.allocate(2048, 2048);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        }
    };

    TEST_F(ThreadPoolAllocatorTest, Test)
    {
        run();
    }

    class ThreadPoolAllocatorDynamicWithStaticPagesTest
        : public ThreadPoolAllocatorTest
    {
    public:
        ThreadPoolAllocatorDynamicWithStaticPagesTest()
            : ThreadPoolAllocatorTest(true, 10) {}                                            // just create 10 pages we will allocate more than
    };

    TEST_F(ThreadPoolAllocatorDynamicWithStaticPagesTest, Test)
    {
        run();
    }

    class ThreadPoolAllocatorStaticPagesTest
        : public ThreadPoolAllocatorTest
    {
    public:
        ThreadPoolAllocatorStaticPagesTest()
            : ThreadPoolAllocatorTest(false, 10000) {}
    };

    TEST_F(ThreadPoolAllocatorStaticPagesTest, Test)
    {
        run();
    }

    AllocationRecordVector::const_iterator FindRecord(const AllocationRecordVector& records, void* ptr)
    {
        return AZStd::find_if(
            records.begin(), records.end(),
            [ptr](const AllocationRecord& record) -> bool
            {
                return record.m_address == ptr;
            });
    }
    /**
     * Tests azmalloc,azmallocex/azfree.
     */
    class AZMallocTest
        : public MemoryTrackingFixture
    {
    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();
        }

        void TearDown() override
        {
            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            SystemAllocator sysAllocator;
            PoolAllocator poolAllocator;

            void* ptr = azmalloc(16*1024, 32, SystemAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (sysAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = sysAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
            }
            azfree(ptr, SystemAllocator);
            if (sysAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = sysAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter == records.end()); // our allocation is in the list
            }
            ptr = azmalloc(16, 32, PoolAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
            }
            azfree(ptr, PoolAllocator);
            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter == records.end());  // our allocation is NOT in the list
            }
        }
    };

    TEST_F(AZMallocTest, Test)
    {
        run();
    }

    /**
     * Tests aznew/delete, azcreate/azdestroy
     */
    class AZNewCreateDestroyTest
        : public MemoryTrackingFixture
    {
        class MyClass
        {
        public:
            AZ_CLASS_ALLOCATOR(MyClass, PoolAllocator, 0);

            MyClass(int data = 303)
                : m_data(data) {}
            ~MyClass() {}

            alignas(32) int m_data;
        };
        // Explicitly doesn't have AZ_CLASS_ALLOCATOR
        class MyDerivedClass
            : public MyClass
        {
        public:
            MyDerivedClass() = default;
        };
    public:
        void SetUp() override
        {
            MemoryTrackingFixture::SetUp();
        }

        void TearDown() override
        {
            MemoryTrackingFixture::TearDown();
        }

        void run()
        {
            SystemAllocator sysAllocator;
            PoolAllocator poolAllocator;

            MyClass* ptr = aznew MyClass(202);  /// this should allocate memory from the pool allocator
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            EXPECT_EQ(202, ptr->m_data);               // check value

            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
            }
            delete ptr;

            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter == records.end()); // our allocation is NOT in the list
            }

            // now use the azcreate to allocate the object wherever we want
            ptr = azcreate(MyClass, (101), SystemAllocator);
            EXPECT_EQ(0, ((size_t)ptr & 31));  // check alignment
            EXPECT_EQ(101, ptr->m_data);               // check value
            if (sysAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = sysAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
            }
            azdestroy(ptr, SystemAllocator);
            if (sysAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = sysAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter == records.end());  // our allocation is NOT in the list
            }

            // Test creation of derived classes
            ptr = aznew MyDerivedClass();       /// this should allocate memory from the pool allocator
            EXPECT_EQ(0, ((size_t)ptr & 31));   // check alignment
            EXPECT_EQ(303, ptr->m_data);        // check value

            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter != records.end());  // our allocation is in the list
            }
            delete ptr;

            if (poolAllocator.GetAllocationCount() > 0)
            {
                AllocationRecordVector records = poolAllocator.GetAllocationRecords();
                AllocationRecordVector::const_iterator iter = FindRecord(records, ptr);
                EXPECT_TRUE(iter == records.end());  // our allocation is NOT in the list
            }
        }
    };

    TEST_F(AZNewCreateDestroyTest, Test)
    {
        run();
    }
}

// GlobalNewDeleteTest-End
