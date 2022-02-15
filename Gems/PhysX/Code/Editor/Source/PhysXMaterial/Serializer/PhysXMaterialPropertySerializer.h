/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    namespace PhysX
    {
        class JsonMaterialPropertySerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(AZ::PhysX::JsonMaterialPropertySerializer, "{F04BF1CA-88AE-4B3D-BB4D-E2D3C5A15E30}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode LoadNumericValues(MaterialTypeSourceData::PropertyDefinition* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Loads a property's value fields from JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode LoadNonNumericValues(MaterialTypeSourceData::PropertyDefinition* intoProperty, const T& defaultValue,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            JsonSerializationResult::ResultCode LoadVectorLabels(MaterialTypeSourceData::PropertyDefinition* intoProperty,
                const rapidjson::Value& inputValue, JsonDeserializerContext& context);

            //! Stores a property's value fields to JSON, for use with numeric types like int and float, which support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode StoreNumericValues(rapidjson::Value& outputValue,
                const MaterialTypeSourceData::PropertyDefinition* property, const T& defaultValue, JsonSerializerContext& context);

            //! Stores a property's value fields to JSON, for use with non-numeric types like Vector and Color, which don't support range limits.
            template<typename T>
            JsonSerializationResult::ResultCode StoreNonNumericValues(rapidjson::Value& outputValue,
                const MaterialTypeSourceData::PropertyDefinition* property, const T& defaultValue, JsonSerializerContext& context);

            JsonSerializationResult::ResultCode StoreVectorLabels(rapidjson::Value& outputValue,
                const MaterialTypeSourceData::PropertyDefinition* property, JsonSerializerContext& context);
        };

    } // namespace PhysX
} // namespace AZ
