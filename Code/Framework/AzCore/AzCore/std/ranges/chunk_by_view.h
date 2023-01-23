/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/std/concepts/concepts.h"
#include "AzCore/std/ranges/ranges.h"
#include "AzCore/std/ranges/reverse_view.h"
#include "AzCore/std/ranges/swap.h"
#include "AzCore/std/ranges/transform_view.h"
#include "AzCore/std/string/string_view.h"
#include "AzCore/std/typetraits/conditional.h"
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/ranges_functional.h>
#include <AzCore/std/ranges/single_view.h>

namespace AZStd::ranges
{
    template<class View, class Predicate, class = enable_if_t<conjunction_v<
        bool_constant<forward_range<View>>,
        bool_constant<AZStd::indirect_binary_predicate<Predicate, iterator_t<View>, iterator_t<View>>>,
        bool_constant<view<View>>,
        bool_constant<is_object_v<Predicate>>
        >>>
        class chunk_by_view;

    // views::chunk_by customization point
    namespace views
    {
        namespace Internal
        {
            struct chunk_by_view_fn
                : Internal::range_adaptor_closure<chunk_by_view_fn>
            {
                template <class View, class Predicate, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view, Predicate&& predicate) const
                {
                    return chunk_by_view(AZStd::forward<View>(view), AZStd::forward<Predicate>(predicate));
                }

                // Create a range_adaptor arugment forwarder which binds the pattern for later
                template <class Predicate, class = enable_if_t<constructible_from<decay_t<Predicate>, Predicate>>>
                constexpr auto operator()(Predicate&& pattern) const
                {
                    return range_adaptor_argument_forwarder(
                        *this, AZStd::forward<Predicate>(pattern));
                }
            };
        } // namespace Internal
        inline namespace customization_point_object
        {
            constexpr Internal::chunk_by_view_fn chunk_by{};
        } // namespace customization_point_object
    } // namespace views

    template <class View, class Predicate, class>
    class chunk_by_view
        : public view_interface<chunk_by_view<View, Predicate>>
    {
    public:
        struct iterator;
        template <bool Enable = default_initializable<View> && default_initializable<Predicate>,
            class = enable_if_t<Enable>>
        constexpr chunk_by_view() {}

        constexpr explicit chunk_by_view(View base, Predicate predicate)
            : m_base(AZStd::move(base))
            , m_predicate(AZStd::move(predicate))
        {
        }

        template <bool Enable = copy_constructible<View>, class = enable_if_t<Enable>>
        constexpr View base() const&
        {
            return m_base;
        }
        constexpr View base() &&
        {
            return AZStd::move(m_base);
        }

        constexpr const Predicate& pred() const
        {
            return m_predicate;
        }

        constexpr iterator begin()
        {
            return { *this, ranges::begin(m_base), find_next(ranges::begin(m_base)) };
        }

        constexpr auto end()
        {
            if constexpr(common_range<View>)
            {
                return iterator{ *this, ranges::end(m_base), ranges::end(m_base) };
            }
            else
            {
                return default_sentinel;
            }
        }

        constexpr iterator_t<View> find_next(iterator_t<View> cur)
        {
            return ranges::next(ranges::adjacent_find(cur, ranges::end(m_base), not_fn(ref(m_predicate))), 1, ranges::end(m_base));
        }
        template <bool E = bidirectional_range<View>, class = enable_if_t<E>>
        constexpr iterator_t<View> find_prev(iterator_t<View> cur)
        {
            using namespace std::placeholders;
            reverse_view rv(subrange(ranges::begin(m_base), cur));
            return ranges::prev(ranges::adjacent_find(rv, not_fn(bind(ref(m_predicate), _2, _1))).base(), 1, ranges::begin(m_base));
        }

    private:
        View m_base;
        Predicate m_predicate;
    };

    template <class View, class Predicate>
    chunk_by_view(View&&, Predicate) -> chunk_by_view<views::all_t<View>, Predicate>;

    template<class View, class Predicate, class Enable>
    struct chunk_by_view<View, Predicate, Enable>::iterator
    {
        using value_type = subrange<iterator_t<View>>;
        using difference_type  = range_difference_t<View>;
        using iterator_category = input_iterator_tag;
        using iterator_concept = AZStd::conditional_t<ranges::bidirectional_range<View>, bidirectional_iterator_tag, forward_iterator_tag>;

        iterator() = default;

        constexpr iterator(chunk_by_view& parent, iterator_t<View> current, iterator_t<View> next)
            : m_parent(&parent)
            , m_current(current)
            , m_next(next)
        {
        }
        constexpr value_type operator*() const
        {
            return { m_current, m_next };
        }
        constexpr iterator& operator++()
        {
            m_current = m_next;
            m_next = m_parent->find_next(m_current);
            return *this;
        }
        constexpr iterator operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        template <bool E = bidirectional_range<View>, class = enable_if_t<E>>
        constexpr iterator& operator--()
        {
            m_next = m_current;
            m_current = m_parent->find_prev(m_next);
            return *this;
        }
        template <bool E = bidirectional_range<View>, class = enable_if_t<E>>
        constexpr iterator operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current;
        }
        friend constexpr bool operator!=(const iterator& x, const iterator& y)
        {
            return x.m_current != y.m_current;
        }
        friend constexpr bool operator==(const iterator& x, default_sentinel_t)
        {
            return x.m_current == x.m_next;
        }

    private:
        chunk_by_view<View, Predicate>* m_parent{};
        iterator_t<View> m_current{};
        iterator_t<View> m_next{};
    };
} // namespace AZStd::ranges
