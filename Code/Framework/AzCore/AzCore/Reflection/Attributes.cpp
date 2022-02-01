/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Reflection/ReflectionConfig.h>

#if AZ_REFLECTION_PROTOTYPE_ENABLED

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Reflection/Attributes.h>
#include <AzCore/std/string/osstring.h>

namespace AZ::Reflection
{
    bool AttributeReader::GetBool(const AttributeDataType* value, bool defaultValue)
    {
        if (value)
        {
            auto converter = [defaultValue](auto&& value) -> bool
            {
                using Type = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<Type, bool>)
                {
                    return value;
                }
                else if constexpr (AZStd::is_same_v<Type, uint64_t> || AZStd::is_same_v<Type, int64_t>)
                {
                    return value != 0;
                }
                else if constexpr (AZStd::is_same_v<Type, double>)
                {
                    return value != 0.0;
                }
                else if constexpr (AZStd::is_same_v<Type, AZStd::string_view>)
                {
                    return value.compare("true") || value.compare("True");
                }
                else
                {
                    return defaultValue;
                }
            };
            return AZStd::visit(converter, *value);
        }
        else
        {
            return defaultValue;
        }
    }

    int64_t AttributeReader::GetInt(const AttributeDataType* value, int64_t defaultValue)
    {
        if (value)
        {
            auto converter = [defaultValue](auto&& value) -> int64_t
            {
                using Type = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<Type, bool>)
                {
                    return value ? 1 : 0;
                }
                else if constexpr (AZStd::is_same_v<Type, uint64_t> || AZStd::is_same_v<Type, double>)
                {
                    return azlossy_cast<int64_t>(value);
                }
                else if constexpr (AZStd::is_same_v<Type, int64_t>)
                {
                    return value;
                }
                else
                {
                    return defaultValue;
                }
            };
            return AZStd::visit(converter, *value);
        }
        else
        {
            return defaultValue;
        }
    }

    AZStd::string_view AttributeReader::GetString(const AttributeDataType* value, AZStd::string_view defaultValue)
    {
        if (value)
        {
            auto converter = [&defaultValue](auto&& value) -> AZStd::string_view
            {
                using Type = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<Type, AZStd::string_view>)
                {
                    return value;
                }
                else if constexpr (AZStd::is_same_v<Type, AZStd::any>)
                {
                    if (auto string = AZStd::any_cast<AZStd::string>(&value); string != nullptr)
                    {
                        AZStd::string_view(string->begin(), string->end());
                    }

                    if (auto string = AZStd::any_cast<AZ::OSString>(&value); string != nullptr)
                    {
                        AZStd::string_view(string->begin(), string->end());
                    }

                    return defaultValue;
                }
                else
                {
                    return defaultValue;
                }
            };
            return AZStd::visit(converter, *value);
        }
        else
        {
            return defaultValue;
        }
    }

    AZ::Uuid AttributeReader::GetUuid([[maybe_unused]] const AttributeDataType* value, [[maybe_unused]] const AZ::Uuid& defaultValue)
    {
        return AZ::Uuid(0);
    }

    const AZStd::any& AttributeReader::GetAny(const AttributeDataType* value, const AZStd::any& defaultValue)
    {
        if (value)
        {
            auto any = AZStd::get_if<AZStd::any>(value);
            return any ? *any : defaultValue;
        }
        else
        {
            return defaultValue;
        }
    }

    bool AttributeReader::GetBool(const IAttributes& attributes, Hash name, bool defaultValue)
    {
        return GetBool(attributes.Find(name, AZ_CRC("group")), defaultValue);
    }

    int64_t AttributeReader::GetInt(const IAttributes& attributes, Hash name, int64_t defaultValue)
    {
        return GetInt(attributes.Find(name, AZ_CRC("group")), defaultValue);
    }

    AZStd::string_view AttributeReader::GetString(const IAttributes& attributes, Hash name, AZStd::string_view defaultValue)
    {
        return GetString(attributes.Find(name, AZ_CRC("group")), defaultValue);
    }

    AZ::Uuid AttributeReader::GetUuid(
        [[maybe_unused]] const IAttributes& attributes, [[maybe_unused]] Hash name, [[maybe_unused]] const AZ::Uuid& defaultValue)
    {
        return AZ::Uuid(0);
    }

    const AZStd::any& AttributeReader::GetAny(
        [[maybe_unused]] const IAttributes& attributes, [[maybe_unused]] Hash name, [[maybe_unused]] const AZStd::any& defaultValue)
    {
        return defaultValue;
    }

    bool AttributeReader::GetBool(const IAttributes& attributes, Hash group, Hash name, bool defaultValue)
    {
        return GetBool(attributes.Find(group, name), defaultValue);
    }

    int64_t AttributeReader::GetInt(const IAttributes& attributes, Hash group, Hash name, int64_t defaultValue)
    {
        return GetInt(attributes.Find(group, name), defaultValue);
    }

    AZStd::string_view AttributeReader::GetString(
        const IAttributes& attributes, Hash group, Hash name, AZStd::string_view defaultValue)
    {
        return GetString(attributes.Find(group, name), defaultValue);
    }

    AZ::Uuid AttributeReader::GetUuid(
        [[maybe_unused]] const IAttributes& attributes,
        [[maybe_unused]] Hash group,
        [[maybe_unused]] Hash name,
        [[maybe_unused]] const AZ::Uuid& defaultValue)
    {
        return AZ::Uuid(0);
    }

    const AZStd::any& AttributeReader::GetAny(
        [[maybe_unused]] const IAttributes& attributes,
        [[maybe_unused]] Hash group,
        [[maybe_unused]] Hash name,
        [[maybe_unused]] const AZStd::any& defaultValue)
    {
        return defaultValue;
    }

    void AttributeReader::ListAttributes(const IAttributes& attributes, const IAttributes::IterationCallback& callback)
    {
        attributes.ListAttributes(callback);
    }

    AttributeCombiner::AttributeCombiner(const IAttributes& first, const IAttributes& second)
        : m_first(first)
        , m_second(second)
    {
    }

    const AttributeDataType* AttributeCombiner::Find(Hash group, Hash name) const
    {
        const AttributeDataType* result = m_first.Find(group, name);
        return result ? result : m_second.Find(group, name);
    }

    void AttributeCombiner::ListAttributes(const IterationCallback& callback) const
    {
        m_second.ListAttributes(callback);
        m_first.ListAttributes(callback);
    }

    void IAttributes::ListAttributes([[maybe_unused]] const IterationCallback& callback) const
    {

    }
} // namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
