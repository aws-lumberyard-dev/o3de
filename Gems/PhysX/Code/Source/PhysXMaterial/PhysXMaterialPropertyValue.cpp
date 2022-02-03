/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXMaterial/PhysXMaterialPropertyValue.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace PhysX
{
    static_assert((
        AZStd::is_same_v<AZStd::monostate,                   AZStd::variant_alternative_t<0, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<bool,                               AZStd::variant_alternative_t<1, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<int32_t,                            AZStd::variant_alternative_t<2, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<uint32_t,                           AZStd::variant_alternative_t<3, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<float,                              AZStd::variant_alternative_t<4, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<AZ::Vector2,                        AZStd::variant_alternative_t<5, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<AZ::Vector3,                        AZStd::variant_alternative_t<6, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<AZ::Vector4,                        AZStd::variant_alternative_t<7, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<AZ::Color,                          AZStd::variant_alternative_t<8, PhysXMaterialPropertyValue::ValueType>> &&
        AZStd::is_same_v<AZStd::string,                      AZStd::variant_alternative_t<9, PhysXMaterialPropertyValue::ValueType>>),
        "Types must be in the order of the type ID array.");

    void PhysXMaterialPropertyValue::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<ValueType>();

            serializeContext->Class<PhysXMaterialPropertyValue>()
                ->Version(1)
                ->Field("Value", &PhysXMaterialPropertyValue::m_value)
                ;
        }
    }

    AZ::TypeId PhysXMaterialPropertyValue::GetTypeId() const
    {
        // Must sort in the same order defined in the variant.
        static const AZ::TypeId PropertyValueTypeIds[] =
        {
            azrtti_typeid<AZStd::monostate>(),
            azrtti_typeid<bool>(),
            azrtti_typeid<int32_t>(),
            azrtti_typeid<uint32_t>(),
            azrtti_typeid<float>(),
            azrtti_typeid<AZ::Vector2>(),
            azrtti_typeid<AZ::Vector3>(),
            azrtti_typeid<AZ::Vector4>(),
            azrtti_typeid<AZ::Color>(),
            azrtti_typeid<AZStd::string>(),
        };

        return PropertyValueTypeIds[m_value.index()];
    }

    PhysXMaterialPropertyValue PhysXMaterialPropertyValue::FromAny(const AZStd::any& value)
    {
        PhysXMaterialPropertyValue result;

        if (value.empty())
        {
            return result;
        }

        if (value.is<bool>())
        {
            result.m_value = AZStd::any_cast<bool>(value);
        }
        else if (value.is<int32_t>())
        {
            result.m_value = AZStd::any_cast<int32_t>(value);
        }
        else if (value.is<uint32_t>())
        {
            result.m_value = AZStd::any_cast<uint32_t>(value);
        }
        else if (value.is<float>())
        {
            result.m_value = AZStd::any_cast<float>(value);
        }
        else if (value.is<double>())
        {
            result.m_value = aznumeric_cast<float>(AZStd::any_cast<double>(value));
        }
        else if (value.is<AZ::Vector2>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector2>(value);
        }
        else if (value.is<AZ::Vector3>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector3>(value);
        }
        else if (value.is<AZ::Vector4>())
        {
            result.m_value = AZStd::any_cast<AZ::Vector4>(value);
        }
        else if (value.is<AZ::Color>())
        {
            result.m_value = AZStd::any_cast<AZ::Color>(value);
        }
        else if (value.is<AZStd::string>())
        {
            result.m_value = AZStd::any_cast<AZStd::string>(value);
        }
        else
        {
            AZ_Warning(
                "MaterialPropertyValue", false, "Cannot convert any to variant. Type in any is: %s.",
                value.get_type_info().m_id.ToString<AZStd::string>().data());
        }

        return result;
    }

    AZStd::any PhysXMaterialPropertyValue::ToAny(const PhysXMaterialPropertyValue& value)
    {
        AZStd::any result;

        if (AZStd::holds_alternative<bool>(value.m_value))
        {
            result = AZStd::get<bool>(value.m_value);
        }
        else if (AZStd::holds_alternative<int32_t>(value.m_value))
        {
            result = AZStd::get<int32_t>(value.m_value);
        }
        else if (AZStd::holds_alternative<uint32_t>(value.m_value))
        {
            result = AZStd::get<uint32_t>(value.m_value);
        }
        else if (AZStd::holds_alternative<float>(value.m_value))
        {
            result = AZStd::get<float>(value.m_value);
        }
        else if (AZStd::holds_alternative<AZ::Vector2>(value.m_value))
        {
            result = AZStd::get<AZ::Vector2>(value.m_value);
        }
        else if (AZStd::holds_alternative<AZ::Vector3>(value.m_value))
        {
            result = AZStd::get<AZ::Vector3>(value.m_value);
        }
        else if (AZStd::holds_alternative<AZ::Vector4>(value.m_value))
        {
            result = AZStd::get<AZ::Vector4>(value.m_value);
        }
        else if (AZStd::holds_alternative<AZ::Color>(value.m_value))
        {
            result = AZStd::get<AZ::Color>(value.m_value);
        }
        else if (AZStd::holds_alternative<AZStd::string>(value.m_value))
        {
            result = AZStd::get<AZStd::string>(value.m_value);
        }

        return result;
    }
} // namespace PhysX
