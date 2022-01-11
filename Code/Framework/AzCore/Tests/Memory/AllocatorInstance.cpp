/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/allocator_stateless.h>

namespace UnitTest
{
    class TestMemoryAllocator // Example Allocator: just a class to use for testing
    {
    public:
        AZ_TYPE_INFO(TestMemoryAllocator, "{EEDC55B9-8E3F-465E-944E-84C76D5F2AB3}");

        // Required to be able to move data from static instances to environment. We could make this optional and those allocators would
        // fail to be used before the environment is ready.
        void Merge(TestMemoryAllocator& aOther) 
        {
            // This is where data from aOther will be moved to "this"
            // For the test we simulate allocations being passed from one allocator to the other
            m_hasAllocations |= aOther.m_hasAllocations;
            aOther.m_hasAllocations = false;
        }

        bool m_hasAllocations = false;
    };
    
    template <class AllocatorType>
    class AllocatorInstancePrototype
    {
    public:
        static inline AllocatorType& Get()  // This is the current method AZ::AllocatorInstance uses to get the global allocator instances
        {
            return GetStatics().Get();
        }
    
    private:
        // Contains all the static data used by AllocatorInstance. Since we want to deal with the member variables using "construct-on-first-use" (lazy
        // initialization), it is convenient to place all the member variables in an inner class that will be created statically on a method of the
        // outer class (refer to GetStatics).
        struct StaticMembers
            : private AZ::Environment::EnvironmentCallback     // Added callback so this object can hook up to the environment attach/detach method
        {
            StaticMembers()
                : m_cachedEnvironmentAllocator(nullptr)
                , m_environmentAllocator()
                , m_environmentAllocatorStorage()
                , m_cachedStaticAllocator(nullptr)
                , m_staticAllocatorStorage()
                , m_allocatorLifetime()
            {}

            virtual ~StaticMembers() = default;
    
            inline AllocatorType& Get()
            {
                if (m_cachedEnvironmentAllocator)
                {
                    // Quickest and most likely path: we have the allocator cached from the environment
                    return *m_cachedEnvironmentAllocator;
                }
    
                return InternalGet();
            }
    
            // Class that controls an allocator's lifetime.
            // Allocator creation scenarios:
            // A1) initially in static memory through static initialization (before AZ::Environment is ready)
            // A2) created for the first time after AZ::Environment is ready
            // Once the AZ::Environment is ready, we transition allocator's data created in (A1) to allocators created
            // by (A2).
            // Allocator destruction will be controlled by AllocatorLifetime, since the environment will also
            // wrap the MemoryAllocator with a AllocatorLifetime, then destruction is ref counted. Two scenarios:
            // D1) if it was created during (A1), the allocator has to be destroyed by static deinitialization.
            //     Static deinitialization order is inverse to static initialization order, since the first variable
            //     that requires allocation will initialize the allocator, we are guaranteed to to get destroyed after
            //     the last variable that requires to deallocate.
            //     The environment will be destroyed first, but since the allocator is managed by ref counting, then it
            //     will be held until the last AllocatorLifetime that holds a copy of the shared_ptr is around.
            // D2) if it was created during (A2), since the environment also holds an AllocatorLifetime, then the
            //     destruction will happen when the environment is destroyed. Since it was created after the environment
            //     was created, it should be safe to destroy it when the environment is destroyed.
            //     If in this scenario, there is a variable that holds an allocation to after the environment is destroyed,
            //     the allocator would be destroyed at that time, which would lead to a leak. The variable's lifetime should
            //     be managed properly to be freed before the allocator is destroyed. If that is not possible, e.g. because
            //     the variable is static and the first allocation happens after the environment is ready, then the variable
            //     should poke the allocator on construction to better define allocator's lifetime.
            //
            // NOTE: AllocatorLifetime is used through AllocatorInstance<AllocatorType>::StaticMembers::m_allocatorLifetime,
            //       which is created in the AllocatorInstance<AllocatorType>::GetStatics() method and called from
            //       AllocatorInstance<AllocatorType>::Get(). Therefore the scope/lifetime is defined by where
            //       AllocatorInstance<AllocatorType>::Get() is invoked from. Since this happens during a heap allocation (before
            //       construction), this will be initialized before the variable that is being created. Because of static
            //       deinitialization order being inverse to static initialization, this AllocatorLifetime will be constructed before
            //       any variable using the allocator and will be destroyed after the variable is destroyed.
            //       
            class AllocatorLifetime
            {
            public:
                void Reset(AllocatorType* memoryAllocator)
                {
                    // We just need to invoke the destructor because the variables are allocated in static storage
                    m_allocator.reset(
                        memoryAllocator,
                        [](AllocatorType* pi)
                        {
                            pi->~AllocatorType();
                        },
                        AZStd::stateless_allocator());
                }
                AZStd::shared_ptr<AllocatorType> m_allocator;
            };
    
            AllocatorType& InternalGet()
            {
                // The most likely case, when the allocator is in the environment and we have a cached version is in the Get method
                // so it gets less i-cache misses

                if (AZ::Environment::IsReady()) // not in the environment, but environment is ready
                {
                    m_environmentAllocator = AZ::Environment::FindVariable<AllocatorLifetime>(AZ::AzTypeInfo<AllocatorType>::Name());
    
                    if (m_environmentAllocator)
                    {
                        // Some other module already created the variable in the environment
                        m_cachedEnvironmentAllocator = m_environmentAllocator.Get().m_allocator.get();
                        m_allocatorLifetime.m_allocator = m_environmentAllocator.Get().m_allocator;
                    }
                    else
                    {
                        // No other module created this allocator, create it
                        m_environmentAllocator = AZ::Environment::CreateVariable<AllocatorLifetime>(AZ::AzTypeInfo<AllocatorType>::Name()); 
                        m_cachedEnvironmentAllocator = new (&m_environmentAllocatorStorage) AllocatorType;
                        m_allocatorLifetime = m_environmentAllocator.Get(); // If there was another allocator (e.g. initialized before environment was ready, it is deleted here)
                        m_allocatorLifetime.Reset(m_cachedEnvironmentAllocator);
                    }

                    if (m_cachedStaticAllocator)
                    {
                        // We currently have a static allocator instance, we move its contents to the environment's one
                        m_cachedEnvironmentAllocator->Merge(*m_cachedStaticAllocator);
                        m_cachedStaticAllocator = nullptr; // It will be destroyed by the assignment to m_allocatorLifetime.m_allocator
                        AZ::Environment::RemoveCallback(this);
                    }
    
                    return *m_cachedEnvironmentAllocator;
                }

                // Allocator not in the environment and environment not ready (less likely scenario)
                // We use a cache variable to hold the static allocator
                if (m_cachedStaticAllocator)
                {
                    return *m_cachedStaticAllocator;
                }

                // No cache variable, need to create the allocator. 
                m_cachedStaticAllocator = new (&m_staticAllocatorStorage) AllocatorType;
                m_allocatorLifetime.Reset(m_cachedStaticAllocator);
                AZ::Environment::AddCallback(this);
       
                return *m_cachedStaticAllocator;
            }
    
            // callback from AZ::Environment during attachment/detachment of the environment.
            void Attached() override
            {
                if (m_cachedStaticAllocator)
                {
                    InternalGet(); // calling this will move the allocator from static to environment variable
                }
            }
            void Created() override
            {
                if (m_cachedStaticAllocator)
                {
                    InternalGet(); // calling this will move the allocator from static to environment variable
                }
            }

            void WillDestroy() override {}
            void WillDetach() override {}   
    
            AllocatorType* m_cachedEnvironmentAllocator;  // pointer to the allocator after it was obtained/created in the environment variable
            AZ::EnvironmentVariable<AllocatorLifetime> m_environmentAllocator; // environment variable that contains the allocator's instance
            typename AZStd::aligned_storage<sizeof(AllocatorType), AZStd::alignment_of<AllocatorType>::value>::type m_environmentAllocatorStorage; // memory storage for the environment allocator
    
            AllocatorType* m_cachedStaticAllocator; // pointer to the allocator it was created in static memory
            typename AZStd::aligned_storage<sizeof(AllocatorType), AZStd::alignment_of<AllocatorType>::value>::type m_staticAllocatorStorage; // memory storage for the static allocator
    
            AllocatorLifetime m_allocatorLifetime; // variable that defines the lifetime of the allocator
        };
    
        static StaticMembers& GetStatics()
        {
            static StaticMembers statics;
            return statics;
        }
    };

    TEST(AllocatorInstance, Create)
    {
        EXPECT_TRUE(AZ::Environment::IsReady());
        AZ::Environment::Destroy();
        EXPECT_FALSE(AZ::Environment::IsReady());

        TestMemoryAllocator& allocatorStatic1 = AllocatorInstancePrototype<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorStatic2 = AllocatorInstancePrototype<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorStatic1, &allocatorStatic2);
        allocatorStatic1.m_hasAllocations = true;

        AZ::Environment::Create(nullptr);
        EXPECT_TRUE(AZ::Environment::IsReady());

        TestMemoryAllocator& allocatorEnvironment1 = AllocatorInstancePrototype<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorEnvironment2 = AllocatorInstancePrototype<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorEnvironment1, &allocatorEnvironment2);

        EXPECT_TRUE(allocatorEnvironment1.m_hasAllocations); // Should have been transferred from allocatorStatic
        EXPECT_FALSE(allocatorStatic1.m_hasAllocations);

    }
}
