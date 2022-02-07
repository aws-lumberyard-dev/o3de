/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/PoolAllocator.h>
#include <AzTest/TestTypes.h>

namespace UnitTest
{
    // Dummy test class
    class TestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, AZ::SystemAllocator, 0);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing the AllocatorsTestFixture base class. Testing that detects leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class AllocatorsTestFixtureLeakDetectionTest
        : public AllocatorsTestFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }
        void TearDown() override
        {
            UnitTest::TestRunner::Instance().m_suppressPrintf = true;
            AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
            const int numAllocators = static_cast<int>(allocatorManager.GetNumAllocators());
            for (int i = numAllocators - 1; i >= 0; --i)
            {
                AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
                allocator->GarbageCollect();
                AZ::IAllocatorTrackingRecorder* allocatorWithTracking = azrtti_cast<AZ::IAllocatorTrackingRecorder*>(allocator);
                if (allocatorWithTracking)
                {
                    allocatorWithTracking->PrintAllocations();
                }
            }
            UnitTest::TestRunner::Instance().m_suppressPrintf = false;
            
            EXPECT_EQ(m_leakExpected, m_leakDetected);
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            if (m_leakExpected)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(2); // one for the SystemAllocator and one for the OSAllocator
            }
            if (m_leakyObject)
            {
                delete m_leakyObject;
                m_leakyObject = nullptr;
            }

            AllocatorsTestFixture::TearDown();
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_leakExpected = true; }

    protected:
        bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override
        {
            AZ_UNUSED(file);
            AZ_UNUSED(line);
            AZ_UNUSED(func);

            if (AZStd::string_view(window) == "Memory"
                && AZStd::string_view(message).starts_with("There are 1 allocations in allocator"))
            {
                // Leak detected, flag it so we validate on tear down that this happened. We also will 
                // mark this test since it will assert
                m_leakDetected = true;
                return true;
            }
            return false;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            // Do not print the error message twice. The error message will already be printed by the TraceBusRedirector
            // in UnitTest.h. Here we override it to prevent it from printing twice.
            return true;
        }

        bool m_leakDetected = false;
        bool m_leakExpected = false;
        TestClass* m_leakyObject = nullptr;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, DISABLED_Leak)
#else
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        SetLeakExpected();

        m_leakyObject = aznew TestClass();
    }

    TEST_F(AllocatorsTestFixtureLeakDetectionTest, NoLeak)
    {
        m_leakyObject = aznew TestClass();
        delete m_leakyObject;
        m_leakyObject = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Allocator leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create a dummy allocator so unit tests can leak it
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(LeakDetection_TestAllocator, AZ::SystemAllocator, "{186B6E32-344D-4322-820A-4C3E4F30650B}")

    // Dummy test class
    class TestClassLeakDetection_TestAllocator
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, LeakDetection_TestAllocator, 0);
    };

    class AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE
        : public ::testing::Test
    {
    public:
        void TestAllocatorLeak()
        {
            [[maybe_unused]] TestClassLeakDetection_TestAllocator* object = new TestClassLeakDetection_TestAllocator();

            // In regular unit test operation, the environment will be teardown at the end and thats where the validation will happen. Here, we need
            // to do a teardown before the test ends so gtest detects the death before it starts to teardown.
            // We suppress the traces so they dont produce more abort calls that would cause the debugger to break (i.e. to stop at a breakpoint). Since
            // this is part of a death test, the trace suppression wont leak because death tests are executed in their own process space.
            AZ_TEST_START_TRACE_SUPPRESSION;
            TraceBusHook* traceBusHook = static_cast<TraceBusHook*>(AZ::Test::sTestEnvironment);
            traceBusHook->TeardownEnvironment();
        }
    };

#if GTEST_HAS_DEATH_TEST
    TEST_F(AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE, AllocatorLeak)
    {
        // testing that the TraceBusHook will fail on cause the test to die
        EXPECT_DEATH(TestAllocatorLeak(), "");
    }
#endif // GTEST_HAS_DEATH_TEST

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Allocators, testing that the different allocator types detect leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename AllocatorType>
    class AllocatorTypeLeakDetectionTest
        : public ::testing::Test
    {
    public:
        // Dummy test class that uses AllocatorType
        class ThisAllocatorTestClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ThisAllocatorTestClass, AllocatorType, 0);
        };

        AllocatorTypeLeakDetectionTest()
            : m_busRedirector(m_leakDetected)
        {}

        void SetUp() override
        {
            m_busRedirector.BusConnect();
        }

        void TearDown() override
        {
            UnitTest::TestRunner::Instance().m_suppressPrintf = true;
            AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
            const int numAllocators = static_cast<int>(allocatorManager.GetNumAllocators());
            for (int i = numAllocators - 1; i >= 0; --i)
            {
                AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
                allocator->GarbageCollect();
                AZ::IAllocatorTrackingRecorder* allocatorWithTracking = azrtti_cast<AZ::IAllocatorTrackingRecorder*>(allocator);
                if (allocatorWithTracking)
                {
                    allocatorWithTracking->PrintAllocations();
                }
            }
            UnitTest::TestRunner::Instance().m_suppressPrintf = false;

            EXPECT_EQ(m_leakExpected, m_leakDetected);
            m_busRedirector.BusDisconnect();
            
            if (m_leakExpected)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
            }
            if (m_leakyObject)
            {
                delete m_leakyObject;
                m_leakyObject = nullptr;
            }
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_leakExpected = true; }

    private:
        class BusRedirector
            : public UnitTest::TraceBusRedirector
        {
        public:
            BusRedirector(bool& leakDetected)
                : m_leakDetected(leakDetected)
            {}

            bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override
            {
                AZ_UNUSED(file);
                AZ_UNUSED(line);
                AZ_UNUSED(func);

                if (AZStd::string_view(window) == "Memory" &&
                    AZStd::string_view(message).starts_with("There are 1 allocations in allocator"))
                {
                    // Leak detected, flag it so we validate on tear down that this happened. We also will 
                    // mark this test since it will assert
                    m_leakDetected = true;
                    return true;
                }
                return false;
            }

            bool OnPrintf(const char* window, const char* message) override
            {
                AZ_UNUSED(window);
                AZ_UNUSED(message);
                // Do not print the error message twice. The error message will already be printed by the TraceBusRedirector
                // in UnitTest.h. Here we override it to prevent it from printing twice.
                return true;
            }

        private:
            bool& m_leakDetected;
        };

    protected:
        BusRedirector m_busRedirector;
        bool m_leakDetected = false;
        bool m_leakExpected = false;
        ThisAllocatorTestClass* m_leakyObject = nullptr;
    };

    using AllocatorTypes = ::testing::Types<
        AZ::SystemAllocator,
        AZ::PoolAllocator,
        AZ::ThreadPoolAllocator
    >;
    TYPED_TEST_CASE(AllocatorTypeLeakDetectionTest, AllocatorTypes);

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TYPED_TEST(AllocatorTypeLeakDetectionTest, DISABLED_Leak)
#else
    TYPED_TEST(AllocatorTypeLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        TestFixture::SetLeakExpected();

        TestFixture::m_leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
    }

    TYPED_TEST(AllocatorTypeLeakDetectionTest, NoLeak)
    {
        TestFixture::m_leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
        delete TestFixture::m_leakyObject;
        TestFixture::m_leakyObject = nullptr;
    }
}


