/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/PhysXMaterial/Serializer/PhysXMaterialPropertyGroupSerializer.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace PhysX
{
    namespace JsonMaterialPropertyGroupSerializerInternal
    {
        namespace Field
        {
            static constexpr const char name[] = "name";
            static constexpr const char id[] = "id"; // For backward compatibility
            static constexpr const char displayName[] = "displayName";
            static constexpr const char description[] = "description";
        }

        static const AZStd::string_view AcceptedFields[] =
        {
            Field::name,
            Field::id,
            Field::displayName,
            Field::description
        };
    }

    AZ_CLASS_ALLOCATOR_IMPL(JsonPhysXMaterialPropertyGroupSerializer, AZ::SystemAllocator, 0);

    AZ::JsonSerializationResult::Result JsonPhysXMaterialPropertyGroupSerializer::Load(void* outputValue, const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertyGroupSerializerInternal;

        AZ_Assert(azrtti_typeid<PhysXMaterialTypeSourceData::GroupDefinition>() == outputValueTypeId,
            "Unable to deserialize material property group to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        PhysXMaterialTypeSourceData::GroupDefinition* propertyGroup = reinterpret_cast<PhysXMaterialTypeSourceData::GroupDefinition*>(outputValue);
        AZ_Assert(propertyGroup, "Output value for JsonMaterialPropertyGroupSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (!inputValue.IsObject())
        {
            return context.Report(AZ::JsonSerializationResult::Tasks::ReadField, AZ::JsonSerializationResult::Outcomes::Unsupported, "Property group must be a JSON object.");
        }
            
        MaterialUtils::CheckForUnrecognizedJsonFields(AcceptedFields, AZ_ARRAY_SIZE(AcceptedFields), inputValue, context, result);
                        
        AZ::JsonSerializationResult::ResultCode nameResult = ContinueLoadingFromJsonObjectField(&propertyGroup->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::name, context);
        if (nameResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::DefaultsUsed)
        {
            // This "id" key is for backward compatibility.
            result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::id, context));
        }
        else
        {
            result.Combine(nameResult);
        }
            
        result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_displayName, azrtti_typeid<AZStd::string>(), inputValue, Field::displayName, context));
        result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_description, azrtti_typeid<AZStd::string>(), inputValue, Field::description, context));

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully loaded property group.");
        }
        else
        {
            return context.Report(result, "Partially loaded property group.");
        }
    }
        

    AZ::JsonSerializationResult::Result JsonPhysXMaterialPropertyGroupSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        [[maybe_unused]] const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertyGroupSerializerInternal;

        AZ_Assert(azrtti_typeid<PhysXMaterialTypeSourceData::GroupDefinition>() == valueTypeId,
            "Unable to serialize material property group to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const PhysXMaterialTypeSourceData::GroupDefinition* propertyGroup = reinterpret_cast<const PhysXMaterialTypeSourceData::GroupDefinition*>(inputValue);
        AZ_Assert(propertyGroup, "Input value for JsonMaterialPropertyGroupSerializer can't be null.");
            
        JSR::ResultCode result(JSR::Tasks::WriteValue);

        outputValue.SetObject();

        AZStd::string defaultEmpty;

        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::name, &propertyGroup->m_name, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::displayName, &propertyGroup->m_displayName, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::description, &propertyGroup->m_description, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully stored property group.");
        }
        else
        {
            return context.Report(result, "Partially stored property group.");
        }
    }
} // namespace PhysX
