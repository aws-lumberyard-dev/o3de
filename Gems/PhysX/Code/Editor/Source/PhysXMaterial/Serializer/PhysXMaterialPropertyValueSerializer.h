/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PhysXMaterial/PhysXMaterialPropertyValue.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace PhysX
{
    class JsonPhysXMaterialPropertyValueSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(PhysX::JsonPhysXMaterialPropertyValueSerializer, "{B1C82FEF-DAEB-45FC-BD76-69D07FB87BCE}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;

    private:
        //! Loads a JSON value into AZStd::variant-based MaterialPropertyValue, using the template data type T
        template<typename T>
        AZ::JsonSerializationResult::ResultCode LoadVariant(PhysXMaterialPropertyValue& intoValue, const T& defaultValue, const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context);
    };
} // namespace PhysX
