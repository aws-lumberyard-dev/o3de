/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PhysXMaterial/PhysXMaterialPropertyValue.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    class JsonDeserializerContext;

    namespace JsonSerializationResult
    {
        union ResultCode;
    }
}

namespace PhysX
{
    class PhysXMaterialTypeSourceData;

    namespace MaterialUtils
    {
        //! Load material type from a json file. If the file path is relative, the loaded json document must be provided.
        //! Otherwise, it will use the passed in document first if not null, or load the json document from the path.
        //! @param filePath a relative path if document is provided, an absolute path if document is not provided.
        //! @param document the loaded json document.
        AZ::Outcome<PhysXMaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, const rapidjson::Value* document = nullptr);

        //! Utility function for custom JSON serializers to report results as "Skipped" when encountering keys that aren't recognized
        //! as part of the custom format.
        //! @param acceptedFieldNames an array of names that are recognized by the custom format
        //! @param acceptedFieldNameCount the number of elements in @acceptedFieldNames
        //! @param object the JSON object being loaded
        //! @param context the common JsonDeserializerContext that is central to the serialization process
        //! @param result the ResultCode that well be updated with the Outcomes "Skipped" if an unrecognized field is encountered
        void CheckForUnrecognizedJsonFields(
            const AZStd::string_view* acceptedFieldNames, uint32_t acceptedFieldNameCount,
            const rapidjson::Value& object, AZ::JsonDeserializerContext& context, AZ::JsonSerializationResult::ResultCode& result);

        //! Materials assets can either be finalized during asset-processing time or when materials are loaded at runtime.
        //! Finalizing during asset processing reduces load times and obfuscates the material data.
        //! Waiting to finalize at load time reduces dependencies on the material type data, resulting in fewer asset rebuilds and less time spent processing assets.
        bool BuildersShouldFinalizeMaterialAssets();
    }
}
