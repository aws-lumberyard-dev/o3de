/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <PhysXMaterial/PhysXMaterialPropertyValue.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace PhysX
{
    class JsonPhysXMaterialPropertySerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonPhysXMaterialPropertySerializer, "{7391F2DE-9D61-4EF5-9465-A7C148BDD631}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;

    private:
        //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
        template<typename T>
        AZ::JsonSerializationResult::ResultCode LoadVariant(PhysXMaterialPropertyValue& intoValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);
        template<typename T>
        AZ::JsonSerializationResult::ResultCode LoadVariant(PhysXMaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

        //! Loads a property's value fields from JSON, for use with numeric types like int and float, which support range limits.
        template<typename T>
        AZ::JsonSerializationResult::ResultCode LoadNumericValues(PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty, const T& defaultValue,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

        //! Loads a property's value fields from JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
        template<typename T>
        AZ::JsonSerializationResult::ResultCode LoadNonNumericValues(PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty, const T& defaultValue,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

        AZ::JsonSerializationResult::ResultCode LoadVectorLabels(PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);

        //! Stores a property's value fields to JSON, for use with numeric types like int and float, which support range limits.
        template<typename T>
        AZ::JsonSerializationResult::ResultCode StoreNumericValues(rapidjson::Value& outputValue,
            const PhysXMaterialTypeSourceData::PropertyDefinition* property, const T& defaultValue, AZ::JsonSerializerContext& context);

        //! Stores a property's value fields to JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
        template<typename T>
        AZ::JsonSerializationResult::ResultCode StoreNonNumericValues(rapidjson::Value& outputValue,
            const PhysXMaterialTypeSourceData::PropertyDefinition* property, const T& defaultValue, AZ::JsonSerializerContext& context);

        AZ::JsonSerializationResult::ResultCode StoreVectorLabels(rapidjson::Value& outputValue,
            const PhysXMaterialTypeSourceData::PropertyDefinition* property, AZ::JsonSerializerContext& context);
    };
} // namespace PhysX
