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

    uint64_t AttributeReader::GetUint(const AttributeDataType* value, uint64_t defaultValue)
    {
        if (value)
        {
            auto converter = [defaultValue](auto&& value) -> uint64_t
            {
                using Type = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<Type, bool>)
                {
                    return value ? 1 : 0;
                }
                else if constexpr (AZStd::is_same_v<Type, int64_t> || AZStd::is_same_v<Type, double>)
                {
                    return azlossy_cast<uint64_t>(value);
                }
                else if constexpr (AZStd::is_same_v<Type, uint64_t>)
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

    double AttributeReader::GetDouble(const AttributeDataType* value, double defaultValue)
    {
        if (value)
        {
            auto converter = [defaultValue](auto&& value) -> double
            {
                using Type = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<Type, bool>)
                {
                    return value ? 1.0 : 0.0;
                }
                else if constexpr (AZStd::is_same_v<Type, int64_t> || AZStd::is_same_v<Type, uint64_t>)
                {
                    return azlossy_cast<double>(value);
                }
                else if constexpr (AZStd::is_same_v<Type, double>)
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

    bool AttributeReader::GetBool(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, bool defaultValue)
    {
        return GetBool(attributes.Find(group, name), defaultValue);
    }

    int64_t AttributeReader::GetInt(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, int64_t defaultValue)
    {
        return GetInt(attributes.Find(group, name), defaultValue);
    }

    uint64_t AttributeReader::GetUint(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, uint64_t defaultValue)
    {
        return GetUint(attributes.Find(group, name), defaultValue);
    }

    double AttributeReader::GetDouble(const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, double defaultValue)
    {
        return GetDouble(attributes.Find(group, name), defaultValue);
    }

    AZStd::string_view AttributeReader::GetString(
        const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, AZStd::string_view defaultValue)
    {
        return GetString(attributes.Find(group, name), defaultValue);
    }

    const AZStd::any& AttributeReader::GetAny(
        const IAttributes& attributes, AZ::Crc32 group, AZ::Crc32 name, const AZStd::any& defaultValue)
    {
        return GetAny(attributes.Find(group, name), defaultValue);
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

    const AttributeDataType* AttributeCombiner::Find(AZ::Crc32 group, AZ::Crc32 name) const
    {
        const AttributeDataType* result = m_first.Find(group, name);
        return result ? result : m_second.Find(group, name);
    }

    void AttributeCombiner::ListAttributes(const IterationCallback& callback) const
    {
        // Prefer listing second's arguments so arguments from the first will override the ones from second.
        m_second.ListAttributes(callback);
        m_first.ListAttributes(callback);
    }
} // namespace AZ::Reflection

#endif // AZ_REFLECTION_PROTOTYPE_ENABLED
