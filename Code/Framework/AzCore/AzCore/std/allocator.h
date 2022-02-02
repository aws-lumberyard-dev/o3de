/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/IAllocator.h>

namespace AZStd
{
    /**
     * \page Allocators
     *
     * AZStd uses simple and fast allocator model, it does not follow the \ref CStd. It is not templated on any type, thus is doesn't care about
     * construction or destruction. We do require
     * name support, in most cases should be just a pointer to a literal.
     *
     * This is the specification for an AZSTD allocator:
     * \code
     * class allocator
     * {
     * public:
     *  // Type of the returned pointer. Usually void*
     *  typedef <impl defined>  pointer_type;
     *  // Size type, any container using this allocator will use this size_type. Usually AZStd::size_t
     *  typedef <impl defined>  size_type;
     *  // Pointer difference type, usually AZStd::ptrdiff_t.
     *  typedef <impl defined>  difference_type;
     *
     *  allocator(const char* name = "AZSTD Allocator");
     *  allocator(const allocator& rhs);
     *  allocator(const allocator& rhs, const char* name);
     *  allocator& operator=(const allocator& rhs;
     *
     *  pointer_type allocate(size_type byteSize, align_type alignment, int flags = 0);
     *  void         deallocate(pointer_type ptr, size_type byteSize, align_type alignment);
     *  /// Tries to resize an existing memory chunk. Returns the resized memory block or 0 if resize is not supported.
     *  size_type    resize(pointer_type ptr, size_type newSize);
     * };
     *
     * bool operator==(const allocator& a, const allocator& b);
     * bool operator!=(const allocator& a, const allocator& b);
     * \endcode
     *
     * \li \ref allocator "Default Allocator"
     * \li \ref no_default_allocator "Invalid Default Allocator"
     * \li \ref static_buffer_allocator "Static Buffer Allocator"
     * \li \ref static_pool_allocator "Static Pool Allocator"
     * \li \ref allocator_ref "Allocator Reference"
     *
     */

    /**
     * All allocation will be piped to AZ::SystemAllocator, make sure it is created!
     */
    class allocator
    {
    public:
        AZ_TYPE_INFO(allocator, "{E9F5A3BE-2B3D-4C62-9E6B-4E00A13AB452}");

        AZ_ALLOCATOR_DEFAULT_TRAITS

        allocator() = default;
        allocator(const allocator& rhs) = default;
        allocator& operator=(const allocator& rhs) = default;

        pointer allocate(size_type byteSize, align_type alignment = 1);
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0);
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);

        size_type max_size() const
        {
            return AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE;
        }

        size_type get_allocated_size() const
        {
            return 0;
        }

        AZ_FORCE_INLINE bool is_lock_free()
        {
            return false;
        }

        AZ_FORCE_INLINE bool is_stale_read_allowed()
        {
            return false;
        }

        AZ_FORCE_INLINE bool is_delayed_recycling()
        {
            return false;
        }
    };

    AZ_FORCE_INLINE bool operator==(const AZStd::allocator&, const AZStd::allocator&)
    {
        return true;
    }

    AZ_FORCE_INLINE bool operator!=(const AZStd::allocator&, const AZStd::allocator&)
    {
        return false;
    }

    /**
    * No Default allocator implementation (invalid allocator).
    * - If you want to make sure we don't use default allocator, define \ref AZStd::allocator to AZStd::no_default_allocator (this allocator)
    *  the code which try to use it, will not compile.
    *
    * - If you have a compile error here, this means that
    * you did not provide allocator to your container. This
    * is intentional so you make sure where do you allocate from. If this is not
    * the AZStd integration this means that you might have defined in your
    * code a default allocator or even have predefined container types. Use them.
    */
    class no_default_allocator
    {
    public:
        AZ_ALLOCATOR_DEFAULT_TRAITS

        AZ_FORCE_INLINE no_default_allocator() = default;
        AZ_FORCE_INLINE no_default_allocator(const allocator&) {}

        // none of this functions are implemented we should get a link error if we use them!
        AZ_FORCE_INLINE allocator& operator=(const allocator& rhs);
        AZ_FORCE_INLINE pointer allocate(size_type byteSize, align_type alignment = 1);
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0);
        AZ_FORCE_INLINE pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);
        AZ_FORCE_INLINE size_type max_size() const;
    };
}
