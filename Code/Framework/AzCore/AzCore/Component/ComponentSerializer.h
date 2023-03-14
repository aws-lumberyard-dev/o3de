/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace AZ
{
    class ComponentSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI(ComponentSerializer, "{31654629-EBF2-4733-845E-69E1B3EA66E5}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        JsonSerializationResult::Result Load(
            void* outputValue,
            const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            const AZ::Uuid& valueTypeId,
            AZ::JsonSerializerContext& context) override;

        JsonSerializationResult::ResultCode ContinueLoadingFromJsonObjectField(
            void* object,
            const Uuid& typeId,
            const rapidjson::Value& value,
            rapidjson::Value::StringRefType memberName,
            JsonDeserializerContext& context,
            ContinuationFlags flags) override;
    };
} // namespace AZ
#pragma once
