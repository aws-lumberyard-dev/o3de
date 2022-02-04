/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/PhysXMaterial/PhysXMaterialSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ReflectContext;

    namespace PhysX
    {
        class JsonMaterialPropertyValueSerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(AZ::PhysX::JsonMaterialPropertyValueSerializer, "{53FCA077-6B3E-48EB-BA8B-945A48E0AD5C}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;

        private:
            //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
            template<typename T>
            JsonSerializationResult::ResultCode LoadVariant(MaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context);
        };

    } // namespace PhysX
} // namespace AZ
