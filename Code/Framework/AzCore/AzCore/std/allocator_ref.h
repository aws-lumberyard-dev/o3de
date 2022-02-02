/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>

namespace AZStd
{
    /**
    * This allocator allows us to share allocator instance between different containers.
    */
    template<class Allocator>
    class allocator_ref
    {
        using this_type = allocator_ref<Allocator>;
    public:
        using value_type = typename Allocator::value_type;
        using pointer = typename Allocator::pointer;
        using size_type = typename Allocator::size_type;
        using difference_type = typename Allocator::difference_type;
        using align_type = typename Allocator::align_type;
        using propagate_on_container_copy_assignment = typename Allocator::propagate_on_container_copy_assignment;
        using propagate_on_container_move_assignment = typename Allocator::propagate_on_container_move_assignment;

        using allocator_pointer = Allocator*;
        using allocator_reference = Allocator&;

        AZ_FORCE_INLINE allocator_ref(allocator_reference allocator)
            : m_allocator(&allocator)
        {
        }
        AZ_FORCE_INLINE allocator_ref(const this_type& rhs)
            : m_allocator(rhs.m_allocator)
        {
        }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
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

        AZ_FORCE_INLINE size_type reallocate(pointer ptr, size_type newSize, align_type alignment = 1)
        {
            return m_allocator->reallocate(ptr, newSize, alignment);
        }

        AZ_FORCE_INLINE allocator_reference get_allocator() const { return *m_allocator; }

        AZ_FORCE_INLINE size_type max_size() const
        {
            return m_allocator->max_size();
        }

        AZ_FORCE_INLINE size_type get_allocated_size() const
        {
            return m_allocator->get_allocated_size();
        }

    private:
        allocator_pointer m_allocator;
    };

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStd::allocator_ref<Allocator>& a, const AZStd::allocator_ref<Allocator>& b)
    {
        return (a.get_allocator() == b.get_allocator());
    }

    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStd::allocator_ref<Allocator>& a, const AZStd::allocator_ref<Allocator>& b)
    {
        return (a.get_allocator() != b.get_allocator());
    }
}


