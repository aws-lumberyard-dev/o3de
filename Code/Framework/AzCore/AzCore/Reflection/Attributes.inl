/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace AZ::Reflection
{
    namespace Internal
    {
        template<typename T>
        auto AttributeTypeApproximation(T&& value)
        {
            using Type = AZStd::decay_t<T>;
            if constexpr (AZStd::is_same_v<Type, bool>)
            {
                return value;
            }
            else if constexpr (AZStd::is_integral_v<Type>)
            {
                if constexpr (AZStd::is_signed_v<Type>)
                {
                    return aznumeric_cast<int64_t>(value);
                }
                else
                {
                    return aznumeric_cast<uint64_t>(value);
                }
            }
            else if constexpr (AZStd::is_floating_point_v<Type>)
            {
                return aznumeric_cast<double>(value);
            }
            else if constexpr (AZStd::is_same_v<Type, const char*>)
            {
                return AZStd::string_view(value);
            }
            else if constexpr (AZStd::is_same_v<Type, AZStd::string_view>)
            {
                return value;
            }
            else
            {
                return AZStd::make_any<Type>(AZStd::forward<T>(value));
            }
        }
    }

    template<size_t N>
    const AttributeDataType* Attributes<N>::Find(AZ::Crc32 group, AZ::Crc32 name) const
    {
        for (size_t i=0; i<N; ++i)
        {
            if (m_groups[i] == group && m_names[i] == name)
            {
                return &m_data[i];
            }
        }
        return nullptr;
    }

    template<size_t N>
    void Attributes<N>::ListAttributes(const IterationCallback& callback) const
    {
        for (size_t i = 0; i < N; ++i)
        {
            callback(m_groups[i], m_names[i], m_data[i]);
        }
    }

    template<typename T>
    auto AttributeArgument(AZ::Crc32 group, AZ::Crc32 name, T&& value)
    {
        auto approximatedValue = Internal::AttributeTypeApproximation(AZStd::forward<T>(value));
        Internal::AttributeArgumentStore<decltype(approximatedValue)> result;
        result.m_group = group;
        result.m_name = name;
        result.m_value = AZStd::move(approximatedValue);
        return result;
    }

    template<typename... Args>
    Attributes<sizeof...(Args)> BuildAttributes(Internal::AttributeArgumentStore<Args>... args)
    {
        Attributes<sizeof...(Args)> result;
        result.m_groups = {args.m_group...};
        result.m_names = {args.m_name...};
        result.m_data = {AZStd::move(args.m_value)...};
        return result;
    }
} // namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED 
