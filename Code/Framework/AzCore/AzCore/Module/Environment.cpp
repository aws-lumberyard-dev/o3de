/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Environment.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    namespace Internal
    {
        void EnvironmentVariableHolderBase::UnregisterAndDestroy(DestructFunc destruct, bool moduleRelease)
        {
            const bool releaseByUseCount = (--m_useCount == 0);
            // We take over the lock, and release it before potentially destroying/freeing ourselves
            {
                AZStd::scoped_lock envLockHolder(AZStd::adopt_lock, m_mutex);
                const bool releaseByModule = (moduleRelease && !m_canTransferOwnership && m_moduleOwner == AZ::Environment::GetModuleId());

                if (!releaseByModule && !releaseByUseCount)
                {
                    return;
                }
                // if the environment that created us is gone the owner can be null
                // which means (assuming intermodule allocator) that the variable is still alive
                // but can't be found as it's not part of any environment.
                if (m_environmentOwner)
                {
                    m_environmentOwner->RemoveVariable(m_guid);
                    m_environmentOwner = nullptr;
                }
                if (m_isConstructed)
                {
                    destruct(this, DestroyTarget::Member); // destruct the value
                }
            }
            // m_mutex is no longer held here, envLockHolder has released it above.
            if (releaseByUseCount)
            {
                // m_mutex is unlocked before this is deleted
                // Call child class dtor and clear the memory
                destruct(this, DestroyTarget::Self);
                AZStd::stateless_allocator().deallocate(this);
            }
        }
       
        // instance of the environment
        EnvironmentInterface* EnvironmentInterface::s_environment = nullptr;

        /**
         *
         */
        class EnvironmentImpl
            : public EnvironmentInterface
        {
        public:
            using MapType = AZStd::unordered_map<u32, void *, AZStd::hash<u32>, AZStd::equal_to<u32>, AZStd::stateless_allocator>;

            static EnvironmentInterface* Get();
            static void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback);
            static void Detach();

            EnvironmentImpl()
                : m_numAttached(0)
                , m_fallback(nullptr)
            {}

            ~EnvironmentImpl() override
            {
                if (m_numAttached > 0)
                {
                    // Is is not reasonable to throw an assert here, as the allocators are likely to have already been torn down
                    // and we will just cause a crash that does not help the user or developer.
                    fprintf(stderr, "We should not delete an environment while there are %d modules attached! Unload all DLLs first!", m_numAttached);
                }

#ifdef AZ_ENVIRONMENT_VALIDATE_ON_EXIT
                AZ_Assert(m_numAttached == 0, "We should not delete an environment while there are %d modules attached! Unload all DLLs first!", m_numAttached);
#endif

                for (const auto &variableIt : m_variableMap)
                {
                    EnvironmentVariableHolderBase* holder = reinterpret_cast<EnvironmentVariableHolderBase*>(variableIt.second);
                    if (holder)
                    {
                        AZ_Assert(static_cast<EnvironmentImpl*>(holder->m_environmentOwner) == this, "We should be the owner of all variables in the map");
                        // since we are going away (no more global look up) remove ourselves, but leave the variable allocated so we can have static variables
                        holder->m_environmentOwner = nullptr;
                    }
                }
            }

            AZStd::recursive_mutex& GetLock() override
            {
                return m_globalLock;
            }

            void AttachFallback(EnvironmentInstance sourceEnvironment) override
            {
                m_fallback = sourceEnvironment;
                if (m_fallback)
                {
                    m_fallback->AddRef();
                }
            }

            void DetachFallback() override
            {
                if (m_fallback)
                {
                    m_fallback->ReleaseRef();
                    m_fallback = nullptr;
                }
            }

            EnvironmentInterface* GetFallback() override
            {
                return m_fallback;
            }

            void* FindVariable(u32 guid) override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    return variableIt->second;
                }
                else
                {
                    return nullptr;
                }
            }

            EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock) override
            {
                EnvironmentVariableResult result;
                m_globalLock.lock();
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end() && variableIt->second != nullptr)
                {
                    result.m_state = EnvironmentVariableResult::Found;
                    result.m_variable = variableIt->second;
                    m_globalLock.unlock();
                    return result;
                }

                if (variableIt == m_variableMap.end()) // if we did not find it or it's not an override look at the fallback
                {
                    if (m_fallback)
                    {
                        AZStd::lock_guard<AZStd::recursive_mutex> fallbackLack(m_fallback->GetLock());
                        void* variable = m_fallback->FindVariable(guid);
                        if (variable)
                        {
                            result.m_state = EnvironmentVariableResult::Found;
                            result.m_variable = variable;
                            m_globalLock.unlock();
                            return result;
                        }
                    }
                }

                auto variableItBool = m_variableMap.insert_key(guid);
                variableItBool.first->second = AZStd::stateless_allocator().allocate(byteSize, alignment);
                if (variableItBool.first->second)
                {
                    result.m_state = EnvironmentVariableResult::Added;
                    result.m_variable = variableItBool.first->second;
                    if (addedVariableLock)
                    {
                        *addedVariableLock = &m_globalLock; // let the user release it
                    }
                    else
                    {
                        m_globalLock.unlock();
                    }
                }
                else
                {
                    result.m_state = EnvironmentVariableResult::OutOfMemory;
                    result.m_variable = nullptr;
                    m_variableMap.erase(variableItBool.first);
                    m_globalLock.unlock();
                }

                return result;
            }

            EnvironmentVariableResult RemoveVariable(u32 guid) override
            {
                // No need to lock, as this function is called already with us owning the lock
                EnvironmentVariableResult result;
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    result.m_state = EnvironmentVariableResult::Removed;
                    result.m_variable = variableIt->second;
                    m_variableMap.erase(variableIt);
                }
                else
                {
                    result.m_state = EnvironmentVariableResult::NotFound;
                    result.m_variable = nullptr;
                }
                return result;
            }

            EnvironmentVariableResult GetVariable(u32 guid) override
            {
                EnvironmentVariableResult result;
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                auto variableIt = m_variableMap.find(guid);
                if (variableIt != m_variableMap.end())
                {
                    if (m_fallback && variableIt->second == nullptr)
                    {
                        // this is an override to ignore the find
                        result.m_state = EnvironmentVariableResult::NotFound;
                        result.m_variable = nullptr;
                    }
                    else
                    {
                        result.m_state = EnvironmentVariableResult::Found;
                        result.m_variable = variableIt->second;
                    }
                }
                else
                {
                    if (m_fallback)
                    {
                        return m_fallback->GetVariable(guid);
                    }
                    else
                    {
                        result.m_state = EnvironmentVariableResult::NotFound;
                        result.m_variable = nullptr;
                    }
                }

                return result;
            }

            void AddRef() override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                m_numAttached++;
            }

            void ReleaseRef() override
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_globalLock);
                m_numAttached--;
            }

            void DeleteThis() override
            {
                this->~EnvironmentImpl();
                AZStd::stateless_allocator().deallocate(this);
            }

            static AZStd::vector<Environment::EnvironmentCallback*, OSAllocator>& GetEnvironmentCallbacks()
            {
                static AZStd::vector<Environment::EnvironmentCallback*, OSAllocator> s_environmentCallbacks;
                return s_environmentCallbacks;
            }

            MapType m_variableMap;

            AZStd::recursive_mutex                m_globalLock;  ///< Mutex that controls access to all environment resources.

            unsigned int                m_numAttached; ///< used for "correctness" checks.
            EnvironmentInterface*       m_fallback;    ///< If set we will use the fallback environment for GetVariable operations.
        };


        /**
        * Destructor will be called when we unload the module.
        */
        class CleanUp
        {
        public:
            CleanUp()
                : m_isOwner(false)
                , m_isAttached(false)
            {
            }
            ~CleanUp()
            {
                if (EnvironmentImpl::s_environment)
                {
                    if (m_isAttached)
                    {
                        EnvironmentImpl::Detach();
                    }
                    if (m_isOwner)
                    {
                        EnvironmentImpl::s_environment->DeleteThis();
                        EnvironmentImpl::s_environment = nullptr;
                    }
                }
            }

            static CleanUp& GetInstance()
            {
                static CleanUp s_environmentCleanUp;
                return s_environmentCleanUp;
            }

            bool m_isOwner;        ///< This is not really needed just to make sure the compiler doesn't optimize the code out.
            bool m_isAttached;     ///< True if this environment is attached in anyway.
        };

        EnvironmentInterface* EnvironmentImpl::Get()
        {
            if (!s_environment)
            {
                Environment::Create();
            }

            return s_environment;
        }

        void EnvironmentImpl::Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback)
        {
            if (!sourceEnvironment)
            {
                return;
            }

            Detach();

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(sourceEnvironment->GetLock());
                if (useAsGetFallback)
                {
                    Get(); // create new environment
                    s_environment->AttachFallback(sourceEnvironment);
                }
                else
                {
                    s_environment = sourceEnvironment;
                    s_environment->AttachFallback(nullptr);
                    s_environment->AddRef();
                }
            }

            CleanUp::GetInstance().m_isAttached = true;
            for (Environment::EnvironmentCallback* callback : GetEnvironmentCallbacks())
            {
                callback->Attached();
            }
        }

        void EnvironmentImpl::Detach()
        {
            if (s_environment && CleanUp::GetInstance().m_isAttached)
            {
                for (Environment::EnvironmentCallback* callback : GetEnvironmentCallbacks())
                {
                    callback->WillDetach();
                }

                AZStd::lock_guard<AZStd::recursive_mutex> lock(s_environment->GetLock());
                if (CleanUp::GetInstance().m_isOwner)
                {
                    if (s_environment->GetFallback())
                    {
                        AZStd::lock_guard<AZStd::recursive_mutex> fallbackLock(s_environment->GetFallback()->GetLock());
                        s_environment->DetachFallback();
                    }
                }

                s_environment->ReleaseRef();
                s_environment = nullptr;

                CleanUp::GetInstance().m_isAttached = false;
            }
        }

        EnvironmentVariableResult AddAndAllocateVariable(u32 guid, size_t byteSize, size_t alignment, AZStd::recursive_mutex** addedVariableLock)
        {
            return EnvironmentImpl::Get()->AddAndAllocateVariable(guid, byteSize, alignment, addedVariableLock);
        }

        EnvironmentVariableResult GetVariable(u32 guid)
        {
            return EnvironmentImpl::Get()->GetVariable(guid);
        }

        u32 EnvironmentVariableNameToId(const char* uniqueName)
        {
            return Crc32(uniqueName);
        }
    } // namespace Internal

    namespace Environment
    {
        bool IsReady()
        {
            return Internal::EnvironmentInterface::s_environment != nullptr;
        }

        void AddCallback(EnvironmentCallback* callback)
        {
            Internal::EnvironmentImpl::GetEnvironmentCallbacks().emplace_back(callback);
        }

        void RemoveCallback(EnvironmentCallback* callback)
        {
            auto& environmentCallbacks = Internal::EnvironmentImpl::GetEnvironmentCallbacks();
            environmentCallbacks.erase(
                AZStd::remove(environmentCallbacks.begin(), environmentCallbacks.end(), callback), environmentCallbacks.end());
        }

        EnvironmentInstance GetInstance()
        {
            return Internal::EnvironmentImpl::Get();
        }

        void* GetModuleId()
        {
            return &Internal::CleanUp::GetInstance();
        }

        bool Create()
        {
            if (Internal::EnvironmentImpl::s_environment)
            {
                return false;
            }

            Internal::EnvironmentImpl::s_environment = new (AZStd::stateless_allocator().allocate(
                sizeof(Internal::EnvironmentImpl), AZStd::alignment_of<Internal::EnvironmentImpl>::value))
                Internal::EnvironmentImpl();
            AZ_Assert(Internal::EnvironmentImpl::s_environment, "We failed to allocate memory from the OS for environment storage %d bytes!", sizeof(Internal::EnvironmentImpl));
            Internal::CleanUp::GetInstance().m_isOwner = true;

            for (Environment::EnvironmentCallback* callback : Internal::EnvironmentImpl::GetEnvironmentCallbacks())
            {
                callback->Created();
            }

            return true;
        }

        void Destroy()
        {
            if (!Internal::CleanUp::GetInstance().m_isAttached && Internal::CleanUp::GetInstance().m_isOwner)
            {
                for (Environment::EnvironmentCallback* callback : Internal::EnvironmentImpl::GetEnvironmentCallbacks())
                {
                    callback->WillDestroy();
                }

                Internal::CleanUp::GetInstance().m_isOwner = false;
                Internal::CleanUp::GetInstance().m_isAttached = false;

                Internal::EnvironmentImpl::s_environment->DeleteThis();
                Internal::EnvironmentImpl::s_environment = nullptr;
            }
            else
            {
                Detach();
            }
        }

        void Detach()
        {
            Internal::EnvironmentImpl::Detach();
        }

        void Attach(EnvironmentInstance sourceEnvironment, bool useAsGetFallback)
        {
            Internal::EnvironmentImpl::Attach(sourceEnvironment, useAsGetFallback);
        }
    } // namespace Environment
} // namespace AZ
