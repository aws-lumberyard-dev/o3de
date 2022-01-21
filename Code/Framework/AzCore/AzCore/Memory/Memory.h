/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/Config.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/has_member_function.h>

/**
 * AZ Memory allocation supports all best know allocation schemes. Even though we highly recommend using the
 * class overriding of new/delete operators for which we provide \ref ClassAllocators. We don't restrict to
 * use whatever you need, each way has it's benefits and drawback. Each of those will be described as we go along.
 * In every macro that doesn't require to specify an allocator AZ::SystemAllocator is implied.
 */
#if !defined(_RELEASE)
    #define aznew                                                   new
    #define aznewex(_Name)                                          new

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)

/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator,#_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3,#_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#else
    #define aznew           new
    #define aznewex(_Name)  new

/// azmalloc(size)
    #define azmalloc_1(_1)                                          AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, 1)
/// azmalloc(size,alignment)
    #define azmalloc_2(_1, _2)                                      AZ::AllocatorInstance< AZ::SystemAllocator >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator)
    #define azmalloc_3(_1, _2, _3)                                  AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName)
    #define azmalloc_4(_1, _2, _3, _4)                              AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)
/// azmalloc(size,alignment,Allocator,allocationName,flags)
    #define azmalloc_5(_1, _2, _3, _4, _5)                          AZ::AllocatorInstance< _3 >::Get().allocate(_1, _2)

/// azcreate(class)
    #define azcreate_1(_1)                                          new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1()
/// azcreate(class,params)
    #define azcreate_2(_1, _2)                                      new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, AZ::SystemAllocator, #_1)) _1 _2
/// azcreate(class,params,Allocator)
    #define azcreate_3(_1, _2, _3)                                  new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, #_1)) _1 _2
/// azcreate(class,params,Allocator,allocationName)
    #define azcreate_4(_1, _2, _3, _4)                              new(azmalloc_4(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4)) _1 _2
/// azcreate(class,params,Allocator,allocationName,flags)
    #define azcreate_5(_1, _2, _3, _4, _5)                          new(azmalloc_5(sizeof(_1), AZStd::alignment_of< _1 >::value, _3, _4, _5)) _1 _2
#endif

/**
* azmalloc is equivalent to ::malloc(...). It should be used with corresponding azfree call.
* macro signature: azmalloc(size_t byteSize, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define azmalloc(...)       AZ_MACRO_SPECIALIZE(azmalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azcalloc(size)
#define azcalloc_1(_1)                  ::memset(azmalloc_1(_1), 0, _1)
/// azcalloc(size, alignment)
#define azcalloc_2(_1, _2)              ::memset(azmalloc_2(_1, _2), 0, _1);
/// azcalloc(size, alignment, Allocator)
#define azcalloc_3(_1, _2, _3)          ::memset(azmalloc_3(_1, _2, _3), 0, _1);
/// azcalloc(size, alignment, allocationName)
#define azcalloc_4(_1, _2, _3, _4)      ::memset(azmalloc_4(_1, _2, _3, _4), 0, _1);
/// azcalloc(size, alignment, allocationName, flags)
#define azcalloc_5(_1, _2, _3, _4, _5)  ::memset(azmalloc_5(_1, _2, _3, _4, _5), 0, _1);

/**
* azcalloc is equivalent to ::memset(azmalloc(...), 0, size);
* macro signature: azcalloc(size, alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator, const char* name = "Default Name", int flags = 0)
*/
#define azcalloc(...)       AZ_MACRO_SPECIALIZE(azcalloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azrealloc(ptr, size)
#define azrealloc_2(_1, _2)             AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(_1, _2, 1)
/// azrealloc(ptr, size, alignment)
#define azrealloc_3(_1, _2, _3)         AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(_1, _2, _3)
/// azrealloc(ptr, size, alignment, Allocator)
#define azrealloc_4(_1, _2, _3, _4)     AZ::AllocatorInstance<_4>::Get().reallocate(_1, _2, _3)

/**
* azrealloc is equivalent to ::realloc(...)
* macro signature: azrealloc(void* ptr, size_t size, size_t alignment = DefaultAlignment, AllocatorType = AZ::SystemAllocator)
*/
#define azrealloc(...)      AZ_MACRO_SPECIALIZE(azrealloc_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * azcreate is customized aznew function call. aznew can be used anywhere where we use new, while azcreate has a function call signature.
 * azcreate allows you to override the operator new and by this you can override the allocator per object instance. It should
 * be used with corresponding azdestroy call.
 * macro signature: azcreate(ClassName, CtorParams = (), AllocatorType = AZ::SystemAllocator, AllocationName = "ClassName", int flags = 0)
 */
#define azcreate(...)       AZ_MACRO_SPECIALIZE(azcreate_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// azfree(pointer)
#define azfree_1(_1)                do { if (_1) { AZ::AllocatorInstance< AZ::SystemAllocator >::Get().deallocate(_1); } }   while (0)
/// azfree(pointer,allocator)
#define azfree_2(_1, _2)            do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1); } }                    while (0)
/// azfree(pointer,allocator,size)
#define azfree_3(_1, _2, _3)        do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1, _3); } }                while (0)
/// azfree(pointer,allocator,size,alignment)
#define azfree_4(_1, _2, _3, _4)    do { if (_1) { AZ::AllocatorInstance< _2 >::Get().deallocate(_1, _3); } }                while (0)

/**
 * azfree is equivalent to ::free(...). Is should be used with corresponding azmalloc call.
 * macro signature: azfree(Pointer* ptr, AllocatorType = AZ::SystemAllocator, size_t byteSize = Unknown, size_t alignment = DefaultAlignment);
 * \note Providing allocation size (byteSize) and alignment is optional, but recommended when possible. It will generate faster code.
 */
#define azfree(...)         AZ_MACRO_SPECIALIZE(azfree_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

namespace AZ {
    // \note we can use AZStd::Internal::destroy<pointer_type>::single(ptr) if we template the entire function.
    namespace Memory {
        namespace Internal {
            template<class T>
            AZ_FORCE_INLINE void call_dtor(T* ptr)          { (void)ptr; ptr->~T(); }
        }
    }
}

#define azdestroy_1(_1)         do { AZ::Memory::Internal::call_dtor(_1); azfree_1(_1); } while (0)
#define azdestroy_2(_1, _2)      do { AZ::Memory::Internal::call_dtor(_1); azfree_2(_1, _2); } while (0)
#define azdestroy_3(_1, _2, _3)     do { AZ::Memory::Internal::call_dtor(reinterpret_cast<_3*>(_1)); azfree_4(_1, _2, sizeof(_3), AZStd::alignment_of< _3 >::value); } while (0)

/**
 * azdestroy should be used only with corresponding azcreate.
 * macro signature: azdestroy(Pointer*, AllocatorType = AZ::SystemAllocator, ClassName = Unknown)
 * \note Providing ClassName is optional, but recommended when possible. It will generate faster code.
 */
#define azdestroy(...)      AZ_MACRO_SPECIALIZE(azdestroy_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/**
 * Class Allocator (override new/delete operators)
 *
 * use this macro inside your objects to define it's default allocation (ex.):
 * class MyObj
 * {
 *   public:
 *          AZ_CLASS_ALLOCATOR(MyObj,SystemAllocator,0) - inline version requires to include the allocator (in this case sysallocator.h)
 *      or
 *          AZ_CLASS_ALLOCATOR_DECL in the header and AZ_CLASS_ALLOCATOR_IMPL(MyObj,SystemAllocator,0) in the cpp file. This way you don't need
 *          to include the allocator header where you decl your class (MyObj).
 *      ...
 * };
 *
 * \note We don't support array operators [] because they insert a compiler/platform
 * dependent array header, which then breaks the alignment in some cases.
 * If you want to use dynamic array use AZStd::vector or AZStd::fixed_vector.
 * Of course you can use placement new and do the array allocation anyway if
 * it's really needed.
 */
static_assert(__cpp_aligned_new);

// _Flag kept for backwards compatibility, not necessary anymore
#define AZ_CLASS_ALLOCATOR(_Class, _Allocator, ...)                                                                                        \
    /* ---------- NEW OPERATORS ----------*/                                                                                               \
    /* replaceable allocation functions */                                                                                                 \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new(AZStd::size_t size)                                                                   \
    {                                                                                                                                      \
        return operator new(size, static_cast<AZStd::align_val_t>(AZStd::alignment_of<_Class>::value));                                    \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new[](AZStd::size_t size)                                                                 \
    {                                                                                                                                      \
        return operator new(size);                                                                                                         \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new(AZStd::size_t size, AZStd::align_val_t al)                                            \
    {                                                                                                                                      \
        /* The >= is necessary because this is used to allocate multiple objects as well, a better check would be */                       \
        /* size % sizeof(_Class) == 0, however, more expensive */                                                                          \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch trying to allocate. Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));     \
        return AZ::AllocatorInstance<_Allocator>::Get().allocate(size, static_cast<_Allocator::align_type>(al));                           \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new[](AZStd::size_t size, AZStd::align_val_t al)                                          \
    {                                                                                                                                      \
        return operator new(size, al);                                                                                                     \
    }                                                                                                                                      \
    /* replaceable non - throwing allocation functions */                                                                                  \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new(AZStd::size_t size, const std::nothrow_t&) noexcept                                   \
    {                                                                                                                                      \
        return operator new(size);                                                                                                         \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new[](AZStd::size_t size, const std::nothrow_t&) noexcept                                 \
    {                                                                                                                                      \
        return operator new[](size);                                                                                                       \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new(AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept            \
    {                                                                                                                                      \
        return operator new(size, al);                                                                                                     \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new[](AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept          \
    {                                                                                                                                      \
        return operator new[](size, al);                                                                                                   \
    }                                                                                                                                      \
    /* non - allocating placement allocation functions */                                                                                  \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new(AZStd::size_t, void* ptr) noexcept                                                    \
    {                                                                                                                                      \
        return ptr;                                                                                                                        \
    }                                                                                                                                      \
    [[nodiscard]] AZ_FORCE_INLINE void* operator new[](AZStd::size_t, void* ptr) noexcept                                                  \
    {                                                                                                                                      \
        return ptr;                                                                                                                        \
    }                                                                                                                                      \
    /* ---------- DELETE OPERATORS ----------*/                                                                                            \
    /* replaceable usual deallocation functions */                                                                                         \
    AZ_FORCE_INLINE void operator delete(void* ptr) noexcept                                                                               \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), static_cast<AZStd::align_val_t>(AZStd::alignment_of<_Class>::value));                         \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr) noexcept                                                                             \
    {                                                                                                                                      \
        operator delete(ptr, AZStd::size_t(0), AZStd::align_val_t(0));                                                                     \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete(void* ptr, AZStd::align_val_t al) noexcept                                                        \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), al);                                                                                          \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr, AZStd::align_val_t al) noexcept                                                      \
    {                                                                                                                                      \
        operator delete(ptr, AZStd::size_t(0), al);                                                                                        \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete(void* ptr, AZStd::size_t sz) noexcept                                                             \
    {                                                                                                                                      \
        operator delete(ptr, sz, AZStd::align_val_t(0));                                                                                   \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr, AZStd::size_t sz) noexcept                                                           \
    {                                                                                                                                      \
        operator delete(ptr, sz, AZStd::align_val_t(0));                                                                                   \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete(void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept                                      \
    {                                                                                                                                      \
        AZ::AllocatorInstance<_Allocator>::Get().deallocate(ptr, sz, static_cast<_Allocator::align_type>(al));                             \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept                                    \
    {                                                                                                                                      \
        operator delete(ptr, sz, al);                                                                                                      \
    }                                                                                                                                      \
    /* replaceable placement deallocation functions */                                                                                     \
    AZ_FORCE_INLINE void operator delete(void* ptr, const std::nothrow_t&) noexcept                                                        \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), AZStd::align_val_t(0));                                                                       \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr, const std::nothrow_t&) noexcept                                                      \
    {                                                                                                                                      \
        operator delete[](ptr, AZStd::size_t(0), AZStd::align_val_t(0));                                                                   \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete(void* ptr, AZStd::align_val_t al, const std::nothrow_t&) noexcept                                 \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), al);                                                                                          \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void* ptr, AZStd::align_val_t al, const std::nothrow_t&) noexcept                               \
    {                                                                                                                                      \
        operator delete[](ptr, AZStd::size_t(0), al);                                                                                      \
    }                                                                                                                                      \
    /* non - allocating placement deallocation functions */                                                                                \
    AZ_FORCE_INLINE void operator delete(void*, void*) noexcept                                                                            \
    {                                                                                                                                      \
    }                                                                                                                                      \
    AZ_FORCE_INLINE void operator delete[](void*, void*) noexcept                                                                          \
    {                                                                                                                                      \
    }                                                                                                                                      \
    template<bool Placeholder = true>                                                                                                      \
    void AZ_CLASS_ALLOCATOR_DECLARED();

#define AZ_CLASS_ALLOCATOR_DECL                                                                                                            \
    /* ---------- NEW OPERATORS ----------*/                                                                                               \
    /* replaceable allocation functions */                                                                                                 \
    [[nodiscard]] void* operator new(AZStd::size_t size);                                                                                  \
    [[nodiscard]] void* operator new[](AZStd::size_t size);                                                                                \
    [[nodiscard]] void* operator new(AZStd::size_t size, AZStd::align_val_t al);                                                           \
    [[nodiscard]] void* operator new[](AZStd::size_t size, AZStd::align_val_t al);                                                         \
    /* replaceable non - throwing allocation functions */                                                                                  \
    [[nodiscard]] void* operator new(AZStd::size_t size, const std::nothrow_t& tag) noexcept;                                              \
    [[nodiscard]] void* operator new[](AZStd::size_t size, const std::nothrow_t& tag) noexcept;                                            \
    [[nodiscard]] void* operator new(AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept;                           \
    [[nodiscard]] void* operator new[](AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept;                         \
    /* non - allocating placement allocation functions */                                                                                  \
    [[nodiscard]] void* operator new(AZStd::size_t, void* ptr) noexcept;                                                                   \
    [[nodiscard]] void* operator new[](AZStd::size_t size, void* ptr) noexcept;                                                            \
    /* ---------- DELETE OPERATORS ----------*/                                                                                            \
    /* replaceable usual deallocation functions */                                                                                         \
    void operator delete(void* ptr) noexcept;                                                                                              \
    void operator delete[](void* ptr) noexcept;                                                                                            \
    void operator delete(void* ptr, AZStd::align_val_t al) noexcept;                                                                       \
    void operator delete[](void* ptr, AZStd::align_val_t al) noexcept;                                                                     \
    void operator delete(void* ptr, AZStd::size_t sz) noexcept;                                                                            \
    void operator delete[](void* ptr, AZStd::size_t sz) noexcept;                                                                          \
    void operator delete(void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept;                                                     \
    void operator delete[](void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept;                                                   \
    /* replaceable placement deallocation functions */                                                                                     \
    void operator delete(void* ptr, const std::nothrow_t& tag) noexcept;                                                                   \
    void operator delete[](void* ptr, const std::nothrow_t& tag) noexcept;                                                                 \
    void operator delete(void* ptr, AZStd::align_val_t al, const std::nothrow_t& tag) noexcept;                                            \
    void operator delete[](void* ptr, AZStd::align_val_t al, const std::nothrow_t& tag) noexcept;                                          \
    /* non - allocating placement deallocation functions */                                                                                \
    void operator delete(void* ptr, void* place) noexcept;                                                                                 \
    void operator delete[](void* ptr, void* place) noexcept;                                                                               \
    template<bool Placeholder = true>                                                                                                      \
    void AZ_CLASS_ALLOCATOR_DECLARED();

// _Flags kept for backwards compatibility, not necessary anymore
#define AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, _Template)                                                                    \
    /* ---------- NEW OPERATORS ----------*/                                                                                               \
    /* replaceable allocation functions */                                                                                                 \
    _Template [[nodiscard]] void* _Class::operator new(AZStd::size_t size)                                                                 \
    {                                                                                                                                      \
        /* The >= is necessary because this is used to allocate multiple objects as well, a better check would be */                       \
        /* size % sizeof(_Class) == 0, however, more expensive */                                                                          \
        AZ_Assert(size >= sizeof(_Class), "Size mismatch trying to allocate. Size: %d sizeof(%s): %d", size, #_Class, sizeof(_Class));     \
        return operator new(size, static_cast<AZStd::align_val_t>(AZStd::alignment_of<_Class>::value));                                    \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new[](AZStd::size_t size)                                                               \
    {                                                                                                                                      \
        return operator new(size);                                                                                                         \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new(AZStd::size_t size, AZStd::align_val_t al)                                          \
    {                                                                                                                                      \
        return AZ::AllocatorInstance<_Allocator>::Get().allocate(size, static_cast<_Allocator::align_type>(al));                           \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new[](AZStd::size_t size, AZStd::align_val_t al)                                        \
    {                                                                                                                                      \
        return operator new(size, al);                                                                                                     \
    }                                                                                                                                      \
    /* replaceable non - throwing allocation functions */                                                                                  \
    _Template [[nodiscard]] void* _Class::operator new(AZStd::size_t size, const std::nothrow_t&) noexcept                                 \
    {                                                                                                                                      \
        return operator new(size);                                                                                                         \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new[](AZStd::size_t size, const std::nothrow_t&) noexcept                               \
    {                                                                                                                                      \
        return operator new[](size);                                                                                                       \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new(AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept          \
    {                                                                                                                                      \
        return operator new(size, al);                                                                                                     \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new[](AZStd::size_t size, AZStd::align_val_t al, const std::nothrow_t&) noexcept        \
    {                                                                                                                                      \
        return operator new[](size, al);                                                                                                   \
    }                                                                                                                                      \
    /* non - allocating placement allocation functions */                                                                                  \
    _Template [[nodiscard]] void* _Class::operator new(AZStd::size_t, void* ptr) noexcept                                                  \
    {                                                                                                                                      \
        return ptr;                                                                                                                        \
    }                                                                                                                                      \
    _Template [[nodiscard]] void* _Class::operator new[](AZStd::size_t, void* ptr) noexcept                                                \
    {                                                                                                                                      \
        return ptr;                                                                                                                        \
    }                                                                                                                                      \
    /* ---------- DELETE OPERATORS ----------*/                                                                                            \
    /* replaceable usual deallocation functions */                                                                                         \
    _Template void _Class::operator delete(void* ptr) noexcept                                                                             \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), AZStd::align_val_t(0));                                                                       \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr) noexcept                                                                           \
    {                                                                                                                                      \
        operator delete(ptr, AZStd::size_t(0), AZStd::align_val_t(0));                                                                     \
    }                                                                                                                                      \
    _Template void _Class::operator delete(void* ptr, AZStd::align_val_t al) noexcept                                                      \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), al);                                                                                          \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr, AZStd::align_val_t al) noexcept                                                    \
    {                                                                                                                                      \
        operator delete(ptr, AZStd::size_t(0), al);                                                                                        \
    }                                                                                                                                      \
    _Template void _Class::operator delete(void* ptr, AZStd::size_t sz) noexcept                                                           \
    {                                                                                                                                      \
        operator delete(ptr, sz, AZStd::align_val_t(0));                                                                                   \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr, AZStd::size_t sz) noexcept                                                         \
    {                                                                                                                                      \
        operator delete(ptr, sz, AZStd::align_val_t(0));                                                                                   \
    }                                                                                                                                      \
    _Template void _Class::operator delete(void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept                                    \
    {                                                                                                                                      \
        AZ::AllocatorInstance<_Allocator>::Get().deallocate(ptr, sz, static_cast<_Allocator::align_type>(al));                             \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr, AZStd::size_t sz, AZStd::align_val_t al) noexcept                                  \
    {                                                                                                                                      \
        operator delete(ptr, sz, al);                                                                                                      \
    }                                                                                                                                      \
    /* replaceable placement deallocation functions */                                                                                     \
    _Template void _Class::operator delete(void* ptr, const std::nothrow_t&) noexcept                                                      \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), AZStd::align_val_t(0));                                                                       \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr, const std::nothrow_t&) noexcept                                                    \
    {                                                                                                                                      \
        operator delete[](ptr, AZStd::size_t(0), AZStd::align_val_t(0));                                                                   \
    }                                                                                                                                      \
    _Template void _Class::operator delete(void* ptr, AZStd::align_val_t al, const std::nothrow_t&) noexcept                               \
    {                                                                                                                                      \
        operator delete(ptr, sizeof(_Class), al);                                                                                          \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void* ptr, AZStd::align_val_t al, const std::nothrow_t&) noexcept                             \
    {                                                                                                                                      \
        operator delete[](ptr, AZStd::size_t(0), al);                                                                                      \
    }                                                                                                                                      \
    /* non - allocating placement deallocation functions */                                                                                \
    _Template void _Class::operator delete(void*, void*) noexcept                                                                          \
    {                                                                                                                                      \
    }                                                                                                                                      \
    _Template void _Class::operator delete[](void*, void*) noexcept                                                                        \
    {                                                                                                                                      \
    }                                                                                                                                      \


#define AZ_CLASS_ALLOCATOR_IMPL(_Class, _Allocator, ...) AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator,)
#define AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(_Class, _Allocator, ...) AZ_CLASS_ALLOCATOR_IMPL_INTERNAL(_Class, _Allocator, template<>)

// you can redefine this macro to whatever suits you.
#ifndef AZCORE_GLOBAL_NEW_ALIGNMENT
    #define AZCORE_GLOBAL_NEW_ALIGNMENT 16
#endif

/**
 * By default AZCore doesn't overload operator new and delete. This is a no-no for middle-ware.
 * You are encouraged to do that in your executable. What you need to do is to pipe all allocation trough AZCore memory manager.
 * AZCore relies on \ref AZ_CLASS_ALLOCATOR to specify the class default allocator or on explicit
 * azcreate/azdestroy calls which provide the allocator. If you class doesn't not implement the
 * \ref AZ_CLASS_ALLOCATOR when you call a new/delete they will use the global operator new/delete. In addition
 * if you call aznew on a class without AZ_CLASS_ALLOCATOR you will need to implement new operator specific to
 * aznew call signature.
 * So in an exception free environment (AZLibs don't have exception support) you need to implement the following functions:
 *
 * void* operator new(AZStd::size_t);
 * void* operator new[](AZStd::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the AZ::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(AZStd::size_t size)         { return AZ::OperatorNew(size); }
 * void* operator new[](AZStd::size_t size)       { return AZ::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { AZ::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { AZ::OperatorDeleteArray(ptr); }
 */
namespace AZ
{
    /**
     * Helper class to determine if type T has a AZ_CLASS_ALLOCATOR defined,
     * so we can safely call aznew on it. -  AZClassAllocator<ClassType>....
     */
    AZ_HAS_MEMBER(AZClassAllocator, AZ_CLASS_ALLOCATOR_DECLARED, void, ());

    // {@ Global New/Delete Operators
    [[nodiscard]] void* OperatorNew(AZStd::size_t size);
    [[nodiscard]] void* OperatorNew(AZStd::size_t size, AZStd::align_val_t align);

    void OperatorDelete(void* ptr);
    void OperatorDelete(void* ptr, AZStd::size_t size);
    void OperatorDelete(void* ptr, AZStd::size_t size, AZStd::align_val_t align);

    [[nodiscard]] void* OperatorNewArray(AZStd::size_t size);
    [[nodiscard]] void* OperatorNewArray(AZStd::size_t size, AZStd::align_val_t align);

    void OperatorDeleteArray(void* ptr);
    void OperatorDeleteArray(void* ptr, AZStd::size_t size);
    void OperatorDeleteArray(void* ptr, AZStd::size_t size, AZStd::align_val_t align);   
    
    // @}
}

#define AZ_PAGE_SIZE AZ_TRAIT_OS_DEFAULT_PAGE_SIZE
#define AZ_DEFAULT_ALIGNMENT (sizeof(void*))

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#define AZ_CORE_MAX_ALLOCATOR_SIZE AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE
