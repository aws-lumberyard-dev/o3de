/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/any.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace PhysX
{
    enum class PhysXMaterialPropertyDataType : uint32_t
    {
        Invalid,

        Bool,
        Int,
        UInt,
        Float,
        Vector2,
        Vector3,
        Vector4,
        Color,
        Enum,   //!< This type is only used in source data files, not runtime data

        Count
    };

    //! This is a variant data type that represents the value of a material property.
    //! For convenience, it supports all the types necessary for *both* the runtime data (MaterialAsset) as well as .material file data (MaterialSourceData).
    //! For example, Instance<Image> is exclusive to the runtime data and AZStd::string is primarily for image file paths in .material files. Most other
    //! data types are relevant in both contexts.
    class PhysXMaterialPropertyValue final
    {
    public:
        AZ_TYPE_INFO(PhysX::PhysXMaterialPropertyValue, "{9598E8BB-7228-4F3E-8685-8911663ED00D}");
        static void Reflect(AZ::ReflectContext* context);

        //! Variant type definition.
        //! AZStd::monostate is used as default and invalid value.
        //! Specially, image type is stored in different type under different contexts:
        //!   AZStd::string           is used in MaterialTypeSourceData, MaterialSourceData
        //! These are included in one MaterialPropertyValue type for convenience, rather than having to maintain three separate classes that are very similar
        using ValueType = AZStd::variant<AZStd::monostate, bool, int32_t, uint32_t, float, AZ::Vector2, AZ::Vector3, AZ::Vector4, AZ::Color, AZStd::string>;

        //! Two-way conversion to AZStd::any
        //! If the type in AZStd::any is not in ValueType, a warning will be reported and AZStd::monostate will be returned.
        static PhysXMaterialPropertyValue FromAny(const AZStd::any& value);
        static AZStd::any ToAny(const PhysXMaterialPropertyValue& value);

        //! Constructors to allow implicit conversions
        PhysXMaterialPropertyValue() = default;
        PhysXMaterialPropertyValue(const bool& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const int32_t& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const uint32_t& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const float& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const AZ::Vector2& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const AZ::Vector3& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const AZ::Vector4& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const AZ::Color& value) : m_value(value) {}
        PhysXMaterialPropertyValue(const AZStd::string& value) : m_value(value) {}

        //! Copy constructor
        PhysXMaterialPropertyValue(const PhysXMaterialPropertyValue& value) : m_value(value.m_value) {}

        //! Templated assignment.
        //! The type will be restricted to those defined in the variant at compile time.
        //! If out-of-definition type is used, the compiler will report error.
        template<typename T>
        void operator=(const T& value)
        {
            m_value = value;
        }

        //! Get actual value from the variant.
        //! The type will be restricted as in operator=().
        template<typename T>
        const T& GetValue() const
        {
            return AZStd::get<T>(m_value);
        }

        //! Check if the type holding is T.
        template<typename T>
        bool Is() const
        {
            return AZStd::holds_alternative<T>(m_value);
        }

        //! Get TypeId of the type holding.
        AZ::TypeId GetTypeId() const;

        //! Check if the variant is holding a valid value.
        bool IsValid() const
        {
            return !AZStd::holds_alternative<AZStd::monostate>(m_value);
        }

        //! Default comparison.
        bool operator== (const PhysXMaterialPropertyValue& other) const
        {
            return m_value == other.m_value;
        }
        bool operator!= (const PhysXMaterialPropertyValue& other) const
        {
            return m_value != other.m_value;
        }

    private:

        ValueType m_value;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(PhysX::PhysXMaterialPropertyDataType, "{1256D217-5211-44DD-8BFC-367F266A2A1D}");
}
