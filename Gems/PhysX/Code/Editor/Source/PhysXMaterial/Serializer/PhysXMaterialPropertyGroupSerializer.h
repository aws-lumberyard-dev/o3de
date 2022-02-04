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
        //! The property group itself is rather simple, but we need this custom serializer to provide backward compatibility
        //! for when the "id" key was changed to "name". If the JSON serialization system is ever updated to provide built-in
        //! support for versioning, then we can probably remove this class.
        class JsonMaterialPropertyGroupSerializer
            : public BaseJsonSerializer
        {
        public:
            AZ_RTTI(JsonMaterialPropertyGroupSerializer, "{C559A9DB-CD37-4AF6-AE71-C72C1D6D0EA9}", BaseJsonSerializer);
            AZ_CLASS_ALLOCATOR_DECL;

            JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
                JsonDeserializerContext& context) override;

            JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue,
                const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context) override;
        };

    } // namespace PhysX
} // namespace AZ
