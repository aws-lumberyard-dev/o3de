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

#pragma once

#include <AzCore/Reflection/ReflectionConfig.h>
#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/Math/Crc.h>
#include <AzCore/std/any.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::Reflection
{
    using AttributeDataType = AZStd::variant<bool, uint64_t, int64_t, double, AZStd::string_view, AZStd::any>;

    struct IAttributes
    {
        using IterationCallback = AZStd::function<void(AZ::Crc32 group, AZ::Crc32 name, const AttributeDataType& attribute)>;

        virtual const AttributeDataType* Find(AZ::Crc32 group, AZ::Crc32 name) const = 0;
        virtual void ListAttributes(const IterationCallback& callback) const = 0;
    };

    namespace Internal
    {
        template<typename T>
        auto AttributeTypeApproximation(T&& value);

        template<typename T>
        struct AttributeArgumentStore
        {
            AZ::Crc32 m_group;
            AZ::Crc32 m_name;
            decltype(Internal::AttributeTypeApproximation(AZStd::declval<T&&>())) m_value;
        };
    }

    struct AttributeReader
    {
        static bool GetBool(const AttributeDataType* value, bool defaultValue);
        static int64_t GetInt(const AttributeDataType* value, int64_t defaultValue);
        static uint64_t GetUint(const AttributeDataType* value, uint64_t defaultValue);
        static double GetDouble(const AttributeDataType* value, double defaultValue);
        static AZStd::string_view GetString(const AttributeDataType* value, AZStd::string_view defaultValue);
        static const AZStd::any& GetAny(const AttributeDataType* value, const AZStd::any& defaultValue);

        static bool GetBool(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, bool defaultValue);
        static int64_t GetInt(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, int64_t defaultValue);
        static uint64_t GetUint(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, uint64_t defaultValue);
        static double GetDouble(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, double defaultValue);
        static AZStd::string_view GetString(
            const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, AZStd::string_view defaultValue);
        static const AZStd::any& GetAny(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, const AZStd::any& defaultValue);

        static void ListAttributes(const IAttributes& attributes, const IAttributes::IterationCallback& callback);
    };

    struct AttributeCombiner : IAttributes
    {
        AttributeCombiner(const IAttributes& first, const IAttributes& second);

        const AttributeDataType* Find(AZ::Crc32 group, AZ::Crc32 name) const override;
        void ListAttributes(const IterationCallback& callback) const override;

        const IAttributes& m_first;
        const IAttributes& m_second;
    };

    template<size_t N>
    struct Attributes : IAttributes
    {
        const AttributeDataType* Find(AZ::Crc32 group, AZ::Crc32 name) const override;
        void ListAttributes(const IterationCallback& callback) const override;

        AZStd::array<AZ::Crc32, N> m_groups;
        AZStd::array<AZ::Crc32, N> m_names;
        AZStd::array<AttributeDataType, N> m_data;
    };

    template<typename T>
    auto AttributeArgument(AZ::Crc32 group, AZ::Crc32 name, T&& value);
    
    template<typename... Args>
    Attributes<sizeof...(Args)> BuildAttributes(Internal::AttributeArgumentStore<Args>... args);
} // namespace AZ::Reflection

#include <AzCore/Reflection/Attributes.inl>

#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
