/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
//       which is created in the AllocatorInstance<AllocatorType>::Get() method and called from
//       AllocatorInstance<AllocatorType>::Get(). Therefore the scope/lifetime is defined by where
//       AllocatorInstance<AllocatorType>::Get() is invoked from. Since this happens during a heap allocation
//       (before construction), this will be initialized before the variable that is being created. Because of
//       static deinitialization order being inverse to static initialization, this AllocatorLifetime will be
//       constructed before any variable using the allocator and will be destroyed after the variable is destroyed.
// NOTE: Allocator vtable transference between modules: suppose A.dll loads B.dll. Suppose Allocator1 is in AzCore
//       which is a static lib, linked by A.dll and B.dll. If B.dll uses Allocator1 first, then the vtable will be
//       pointing to the Allocator1's symbols from B.dll. If B.dll is unloaded, then A.dll wont be able to use the
//       allocator.
//       TODO: To solve this, we track the Module that created the allocator, when a module is about to be unloaded,
//       we make the module that is going to perform the unload to go through all allocators and check if any
//       allocator belongs to that module that is going to be unloaded, if it does, create a new allocator from the
//       module that will trigger the unload (A.dll in our example) and transfer the allocations.
//

namespace AZ
{
    namespace Internal
    {
        template<class AllocatorType>
        class AllocatorInstanceStaticMembers;
    }

    template<class AllocatorType>
    class AllocatorInstance
    {
    public:
        static AllocatorType& Get(); // This is the current method AZ::AllocatorInstance uses to get the global allocator instances

        // Kept for backwards compatibility
        ////////////////////////////////////////////
        static bool IsReady()
        {
            return true;
        }

        static void Create()
        {
        }

        static void Destroy()
        {
        }
        ////////////////////////////////////////////
    };
}

#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Module/Environment.h>

namespace AZ::Internal
{
    // Contains all the static data used by AllocatorInstance. Since we want to deal with the member variables using
    // "construct-on-first-use" (lazy initialization), it is convenient to place all the member variables in an inner class that will be
    // created statically on a method of the outer class (refer to Get).
    template<class AllocatorType>
    class AllocatorInstanceStaticMembers : private AZ::Environment::EnvironmentCallback // Added callback so this object can hook up to the
                                                                                        // environment attach/detach method
    {
    public:
        AllocatorInstanceStaticMembers()
            : m_cachedEnvironmentAllocator(nullptr)
            , m_environmentAllocator()
            , m_cachedStaticAllocator(nullptr)
            , m_allocatorLifetime()
        {
        }

        virtual ~AllocatorInstanceStaticMembers() = default;

        inline AllocatorType& Get()
        {
            if (m_cachedEnvironmentAllocator)
            {
                // Quickest and most likely path: we have the allocator cached from the environment
                return *m_cachedEnvironmentAllocator;
            }

            return InternalGet();
        }

    private:
        class AllocatorLifetime
        {
        public:
            void Create()
            {
                m_data.reset(
                    new (reinterpret_cast<Data*>(AZStd::stateless_allocator().allocate(sizeof(Data), alignof(Data)))) Data(),
                    [](Data* pi) { pi->~Data(); },
                    AZStd::stateless_allocator());
            }
            AllocatorType* GetAllocator()
            {
                AZ_Assert(m_data, "Invalid data in AllocatorLifetime");
                return m_data->GetAllocator();
            }
            bool IsValid() const
            {
                return m_data;
            }
            void Reset()
            {
                m_data.reset();
            }
        private:
            struct Data
            {
                Data()
                {
                    new (&m_allocatorStorage) AllocatorType;
                }

                ~Data()
                {
                    GetAllocator()->~AllocatorType(); // no need to free the memory since is stored in m_allocatorStorage
                }

                AllocatorType* GetAllocator()
                {
                    return reinterpret_cast<AllocatorType*>(&m_allocatorStorage);
            }

        private:
                using AllocatorStorage = typename AZStd::aligned_storage<sizeof(AllocatorType), alignof(AllocatorType)>::type;
                AllocatorStorage m_allocatorStorage; // memory storage for the environment allocator
            };
            AZStd::shared_ptr<Data> m_data;
        };

        AllocatorType& InternalGet()
        {
            // The most likely case, when the allocator is in the environment and we have a cached version is in the Get method
            // so it gets less i-cache misses

            if (AZ::Environment::IsReady()) // not in the environment, but environment is ready
            {
                m_environmentAllocator = AZ::Environment::FindVariable<AllocatorLifetime>(AZ::AzTypeInfo<AllocatorType>::Name());

                AllocatorLifetime currentLifetime = m_allocatorLifetime; // in case there was a static allocator

                if (m_environmentAllocator)
                {
                    // Some other module already created the variable in the environment
                    m_allocatorLifetime = m_environmentAllocator.Get();
                    m_cachedEnvironmentAllocator = m_allocatorLifetime.GetAllocator();
                    // Dont reset currentLifetime, we have to transfer allocations from there to the allocator
                    // we got from the environment
                }
                else if (currentLifetime.IsValid())
                {
                    // this module created the allocator before the environment was ready, now that the environment
                    // is ready, set the environment to the allocator we already created
                    m_environmentAllocator = AZ::Environment::CreateVariable<AllocatorLifetime>(AZ::AzTypeInfo<AllocatorType>::Name());
                    m_environmentAllocator.Get() = m_allocatorLifetime;
                    m_cachedEnvironmentAllocator = m_allocatorLifetime.GetAllocator();
                    m_cachedStaticAllocator = nullptr;
                    currentLifetime.Reset(); // we reset here because we are reusing the allocator from the static case

                    AZ::AllocatorManager::Instance().RegisterAllocator(m_cachedEnvironmentAllocator);
                }
                else
                {
                    // We currently dont have an allocator, the environment is ready so create it
                    AZ_Assert(m_cachedStaticAllocator == nullptr, "Didn't expect to have a static allocator")
                    m_allocatorLifetime.Create();

                    m_environmentAllocator = AZ::Environment::CreateVariable<AllocatorLifetime>(AZ::AzTypeInfo<AllocatorType>::Name());
                    m_environmentAllocator.Get() = m_allocatorLifetime;
                    m_cachedEnvironmentAllocator = m_allocatorLifetime.GetAllocator();

                    AZ::AllocatorManager::Instance().RegisterAllocator(m_cachedEnvironmentAllocator);
                }

                // If we reach this point is because we had a static allocator and when the environment became available, we
                // got another allocator from the environment (likely created by another module). In this case, we still have
                // to move allocations (Merge) from the static allocator into the one from the environment
                if (currentLifetime.IsValid())
                {
                    // We should only have the static allocator in currentLifetime
                    AZ_Assert(m_cachedStaticAllocator == currentLifetime.GetAllocator(), "Expected to have the static allocator in the lifetime variable");
                    if (m_cachedEnvironmentAllocator != m_cachedStaticAllocator)
                    {
                        // We currently have a static allocator instance, we move its contents to the environment's one
                        m_cachedEnvironmentAllocator->Merge(m_cachedStaticAllocator);
                    }                   
                    m_cachedStaticAllocator = nullptr; // It will be destroyed by the assignment to m_allocatorLifetime.m_allocator

                    // The static allocator instance will be deleted when the variable currentLifetime goes out of scope
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
            AZ_Assert(!m_allocatorLifetime.IsValid(), "Allocator lifetime should be invalid");
            m_allocatorLifetime.Create();
            m_cachedStaticAllocator = m_allocatorLifetime.GetAllocator(); // cache it for the next time

            AZ::Environment::AddCallback(this);

            return *m_cachedStaticAllocator;
        }

        // callback from AZ::Environment during attachment/detachment of the environment.
        void Attached() override
        {
            Created();
        }
        void Created() override
        {
            if (!m_cachedEnvironmentAllocator)
            {
                InternalGet(); // calling this will move the allocator from static to environment variable
            }
        }

        void WillDestroy() override
        {
            WillDetach();
        }
        void WillDetach() override
        {
            m_cachedEnvironmentAllocator = nullptr;
            if (!m_cachedStaticAllocator)
            {
                if (m_allocatorLifetime.IsValid())
                {
                    m_cachedStaticAllocator = m_allocatorLifetime.GetAllocator();
                }
                else
                {
                    m_cachedStaticAllocator = nullptr;
                }
            }
        }

        AllocatorType* m_cachedEnvironmentAllocator; // pointer to the allocator after it was obtained/created in the environment variable
        AllocatorType* m_cachedStaticAllocator; // pointer to the allocator it was created in static memory

        AllocatorLifetime m_allocatorLifetime; // variable that defines the lifetime of the allocator
        AZ::EnvironmentVariable<AllocatorLifetime> m_environmentAllocator; // environment variable that contains the allocator's instance
    };
}

namespace AZ
{
    template<class AllocatorType>
    AllocatorType& AllocatorInstance<AllocatorType>::Get() // This is the current method AZ::AllocatorInstance uses to get the global allocator instances
    {
        static Internal::AllocatorInstanceStaticMembers<AllocatorType> statics;
        return statics.Get();
    }

}
