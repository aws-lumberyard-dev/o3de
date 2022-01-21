/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    /**
    * Generic wrapper for binding allocator to an AZStd one.
    * \note AZStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template <class Allocator>
    class AZStdAlloc
    {
    public:
        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;         ///< Regular allocators should not leak.

        AZ_FORCE_INLINE AZStdAlloc()
        {
            if (AllocatorInstance<Allocator>::IsReady())
            {
                m_name = AllocatorInstance<Allocator>::Get().GetName();
            }
            else
            {
                m_name = AzTypeInfo<Allocator>::Name();
            }
        }
        AZ_FORCE_INLINE AZStdAlloc(const char* name)
            : m_name(name)     {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs)
            : m_name(rhs.m_name)  {}
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs, const char* name)
            : m_name(name) { (void)rhs; }
        AZ_FORCE_INLINE AZStdAlloc& operator=(const AZStdAlloc& rhs) { m_name = rhs.m_name; return *this; }
        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return AllocatorInstance<Allocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type ptr, size_type newSize)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize);
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().DeAllocate(ptr, byteSize, alignment);
        }
    private:
        const char* m_name;
    };

    AZ_TYPE_INFO_TEMPLATE(AZStdAlloc, "{42D0AA1E-3C6C-440E-ABF8-435931150470}", AZ_TYPE_INFO_CLASS);

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return true; } // always true since they use the same instance of AllocatorInstance<Allocator>
    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return false; } // always false since they use the same instance of AllocatorInstance<Allocator>

    /**
     * Generic wrapper for binding IAllocator interface allocator.
     * This is basically the same as \ref AZStdAlloc but it allows
     * you to remove the template parameter and set you interface on demand.
     * of course at a cost of a pointer.
     */
    class AZStdIAllocator
    {
    public:
        using value_type = void;
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using align_type = AZStd::size_t;
        using propagate_on_container_move_assignment = AZStd::true_type;

        AZ_FORCE_INLINE AZStdIAllocator(IAllocator* allocator)
            : m_allocator(allocator)
        {
            AZ_Assert(m_allocator != NULL, "You must provide a valid allocator!");
        }
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs)
            : m_allocator(rhs.m_allocator)
        {
        }
        AZ_FORCE_INLINE AZStdIAllocator& operator=(const AZStdIAllocator& rhs)
        {
            m_allocator = rhs.m_allocator;
            return *this;
        }
        AZ_FORCE_INLINE pointer allocate(size_type byteSize, align_type alignment = 1)
        {
            return m_allocator->allocate(byteSize, alignment);
        }
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0)
        {
            m_allocator->deallocate(ptr, byteSize, alignment);
        }
        AZ_FORCE_INLINE bool operator==(const AZStdIAllocator& rhs) const
        {
            return m_allocator == rhs.m_allocator;
        }
        AZ_FORCE_INLINE bool operator!=(const AZStdIAllocator& rhs) const
        {
            return m_allocator != rhs.m_allocator;
        }

    private:
        IAllocator* m_allocator;
    };

    /**
    * Generic wrapper for binding IAllocator interface allocator.
    * This is basically the same as \ref AZStdAlloc but it allows
    * you to remove the template parameter and retrieve the allocator from a supplied function
    * pointer
    */
    class AZStdFunctorAllocator
    {
    public:
        using pointer_type = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using allow_memory_leaks = AZStd::false_type; ///< Regular allocators should not leak.
        using functor_type = IAllocator&(*)(); ///< Function Pointer must return IAllocator&.
                                               ///< function pointers do not support covariant return types

        constexpr AZStdFunctorAllocator(functor_type allocatorFunctor, const char* name = "AZ::AZStdFunctorAllocator")
            : m_allocatorFunctor(allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr AZStdFunctorAllocator(const AZStdFunctorAllocator& rhs, const char* name)
            : m_allocatorFunctor(rhs.m_allocatorFunctor)
            , m_name(name)
        {
        }
        constexpr AZStdFunctorAllocator(const AZStdFunctorAllocator& rhs) = default;
        constexpr AZStdFunctorAllocator& operator=(const AZStdFunctorAllocator& rhs) = default;
        pointer_type allocate(size_t byteSize, size_t alignment, int flags = 0)
        {
            return m_allocatorFunctor().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }
        size_type resize(pointer_type ptr, size_t newSize)
        {
            return m_allocatorFunctor().Resize(ptr, newSize);
        }
        void deallocate(pointer_type ptr, size_t byteSize, size_t alignment)
        {
            m_allocatorFunctor().DeAllocate(ptr, byteSize, alignment);
        }
        constexpr const char* get_name() const { return m_name; }
        void set_name(const char* name) { m_name = name; }

        constexpr bool operator==(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor == rhs.m_allocatorFunctor; }
        constexpr bool operator!=(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor != rhs.m_allocatorFunctor; }
    private:
        functor_type m_allocatorFunctor;
        const char* m_name;
    };
}
