/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>

namespace PhysX
{
    //! The property group itself is rather simple, but we need this custom serializer to provide backward compatibility
    //! for when the "id" key was changed to "name". If the JSON serialization system is ever updated to provide built-in
    //! support for versioning, then we can probably remove this class.
    class JsonPhysXMaterialPropertyGroupSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(JsonPhysXMaterialPropertyGroupSerializer, "{C653D97A-EBEF-4840-9D83-49D4DF7FB32F}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override;

        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override;
    };
} // namespace AZ
