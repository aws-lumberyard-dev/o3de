/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzCore/Name/Name.h>


namespace AZ::Reflection
{
    using AttributeDataType = AZStd::variant<bool, uint64_t, int64_t, double, AZStd::string_view, AZ::Uuid, AZStd::any>;
    using Hash = AZ::Name::Hash;

    struct IAttributes
    {
        using IterationCallback = AZStd::function<void(AZStd::string_view group, AZStd::string_view name, const AttributeDataType& attribute)>;
        virtual const AttributeDataType* Find(Hash name) const;
        virtual const AttributeDataType* Find(Hash group, Hash name) const;
        virtual void ListAttributes(const IterationCallback& callback) const;
    };

    namespace Internal
    {
        template<typename T>
        auto AttributeTypeApproximation(T&& value);

        template<typename T>
        struct AttributeArgumentStore
        {
            Hash m_group;
            Hash m_name;
            decltype(Internal::AttributeTypeApproximation(AZStd::declval<T&&>())) m_value;
        };
    }

    struct AttributeReader
    {
        static bool GetBool(const AttributeDataType* value, bool defaultValue);
        static int64_t GetInt(const AttributeDataType* value, int64_t defaultValue);
        static AZStd::string_view GetString(const AttributeDataType* value, AZStd::string_view defaultValue);
        static AZ::Uuid GetUuid(const AttributeDataType* value, const AZ::Uuid& defaultValue);
        static const AZStd::any& GetAny(const AttributeDataType* value, const AZStd::any& defaultValue);

        static bool GetBool(const IAttributes& attributes, Hash name, bool defaultValue);
        static int64_t GetInt(const IAttributes& attributes, Hash name, int64_t defaultValue);
        static AZStd::string_view GetString(const IAttributes& attributes, Hash name, AZStd::string_view defaultValue);
        static AZ::Uuid GetUuid(const IAttributes& attributes, Hash name, const AZ::Uuid& defaultValue);
        static const AZStd::any& GetAny(const IAttributes& attributes, Hash name, const AZStd::any& defaultValue);

        static bool GetBool(const IAttributes& attributes, Hash group, Hash name, bool defaultValue);
        static int64_t GetInt(const IAttributes& attributes, Hash group, Hash name, int64_t defaultValue);
        static AZStd::string_view GetString(const IAttributes& attributes, Hash group, Hash name, AZStd::string_view defaultValue);
        static AZ::Uuid GetUuid(const IAttributes& attributes, Hash group, Hash name, const AZ::Uuid& defaultValue);
        static const AZStd::any& GetAny(const IAttributes& attributes, Hash group, Hash name, const AZStd::any& defaultValue);

        static void ListAttributes(const IAttributes& attribtutes, const IAttributes::IterationCallback& callback);
    };

    struct AttributeCombiner : IAttributes
    {
        AttributeCombiner(const IAttributes& first, const IAttributes& second);
        const AttributeDataType* Find(Hash group, Hash name) const override;
        void ListAttributes(const IterationCallback& callback) const override;

        const IAttributes& m_first;
        const IAttributes& m_second;
    };

    template<size_t N>
    struct Attributes : IAttributes
    {
        const AttributeDataType* Find(Hash group, Hash name) const override;
        void ListAttributes(const IterationCallback& callback) const override;

        AZStd::array<Hash, N> m_groups;
        AZStd::array<Hash, N> m_names;
        AZStd::array<AttributeDataType, N> m_data;
    };

    template<typename T>
    auto AttributeArgument(Hash group, Hash name, T&& value);

    template<typename... Args>
    Attributes<sizeof...(Args)> BuildAttributes(Internal::AttributeArgumentStore<Args>... args);
}

#include <AzCore/Reflection/Attributes.inl>

#endif //AZ_REFLECTION_PROTOTYPE_ENABLED
