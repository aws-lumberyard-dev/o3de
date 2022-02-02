/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/PhysXMaterial/Serializer/PhysXMaterialPropertySerializer.h>
//#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
//#include <Atom/RPI.Edit/Material/MaterialUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace PhysX
{
    namespace JsonMaterialPropertySerializerInternal
    {
        namespace Field
        {
            static constexpr const char name[] = "name";
            static constexpr const char id[] = "id"; // For backward compatibility
            static constexpr const char displayName[] = "displayName";
            static constexpr const char description[] = "description";
            static constexpr const char type[] = "type";
            static constexpr const char visibility[] = "visibility";
            static constexpr const char defaultValue[] = "defaultValue";
            static constexpr const char min[] = "min";
            static constexpr const char max[] = "max";
            static constexpr const char softMin[] = "softMin";
            static constexpr const char softMax[] = "softMax";
            static constexpr const char step[] = "step";
            static constexpr const char connection[] = "connection";
            static constexpr const char enumValues[] = "enumValues";
            static constexpr const char vectorLabels[] = "vectorLabels";
        }

        static const AZStd::string_view AcceptedFields[] =
        {
            Field::name,
            Field::id,
            Field::displayName,
            Field::description,
            Field::type,
            Field::visibility,
            Field::defaultValue,
            Field::min,
            Field::max,
            Field::softMin,
            Field::softMax,
            Field::step,
            Field::connection,
            Field::enumValues,
            Field::vectorLabels
        };
    }

    AZ_CLASS_ALLOCATOR_IMPL(JsonPhysXMaterialPropertySerializer, AZ::SystemAllocator, 0);


    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::LoadVariant(
        PhysXMaterialPropertyValue& intoValue,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        T value;
        AZ::JsonSerializationResult::ResultCode result = ContinueLoading(&value, azrtti_typeid<T>(), inputValue, context);
        if (result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success)
        {
            intoValue = value;
        }
        return result;
    }

    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::LoadVariant(
        PhysXMaterialPropertyValue& intoValue,
        const T& defaultValue,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        T value = defaultValue;
        AZ::JsonSerializationResult::ResultCode result = ContinueLoading(&value, azrtti_typeid<T>(), inputValue, context);
        intoValue = value;
        return result;
    }

    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::LoadNumericValues(
        PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty,
        const T& defaultValue,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (inputValue.HasMember(Field::defaultValue))
        {
            AZ::ScopedContextPath subPath{context, Field::defaultValue};
            result.Combine(LoadVariant<T>(intoProperty->m_value, defaultValue, inputValue[Field::defaultValue], context));
        }
        else
        {
            intoProperty->m_value = defaultValue;
            result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::PartialDefaults));
        }

        // The following do not report PartialDefault because when these are omitted/null the data in the property will also be null

        if (inputValue.HasMember(Field::min))
        {
            AZ::ScopedContextPath subPath{context, Field::min};
            result.Combine(LoadVariant<T>(intoProperty->m_min, inputValue[Field::min], context));
        }

        if (inputValue.HasMember(Field::max))
        {
            AZ::ScopedContextPath subPath{context, Field::max};
            result.Combine(LoadVariant<T>(intoProperty->m_max, inputValue[Field::max], context));
        }

        if (inputValue.HasMember(Field::softMin))
        {
            AZ::ScopedContextPath subPath{ context, Field::softMin };
            result.Combine(LoadVariant<T>(intoProperty->m_softMin, inputValue[Field::softMin], context));
        }

        if (inputValue.HasMember(Field::softMax))
        {
            AZ::ScopedContextPath subPath{ context, Field::softMax };
            result.Combine(LoadVariant<T>(intoProperty->m_softMax, inputValue[Field::softMax], context));
        }

        if (inputValue.HasMember(Field::step))
        {
            AZ::ScopedContextPath subPath{context, Field::step};
            result.Combine(LoadVariant<T>(intoProperty->m_step, inputValue[Field::step], context));
        }

        return result;
    }

    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::LoadNonNumericValues(
        PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty,
        const T& defaultValue,
        const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (inputValue.HasMember(Field::defaultValue))
        {
            AZ::ScopedContextPath subPath{context, Field::defaultValue};
            result.Combine(LoadVariant<T>(intoProperty->m_value, defaultValue, inputValue[Field::defaultValue], context));
        }
        else
        {
            intoProperty->m_value = defaultValue;
            result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::PartialDefaults));
        }

        return result;
    }

    AZ::JsonSerializationResult::Result JsonPhysXMaterialPropertySerializer::Load(void* outputValue, const AZ::Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        AZ_Assert(azrtti_typeid<PhysXMaterialTypeSourceData::PropertyDefinition>() == outputValueTypeId,
            "Unable to deserialize material property to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);

        PhysXMaterialTypeSourceData::PropertyDefinition* property = reinterpret_cast<PhysXMaterialTypeSourceData::PropertyDefinition*>(outputValue);
        AZ_Assert(property, "Output value for JsonMaterialPropertySerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (!inputValue.IsObject())
        {
            return context.Report(AZ::JsonSerializationResult::Tasks::ReadField, AZ::JsonSerializationResult::Outcomes::Unsupported, "Property definition must be a JSON object.");
        }

        MaterialUtils::CheckForUnrecognizedJsonFields(AcceptedFields, AZ_ARRAY_SIZE(AcceptedFields), inputValue, context, result);

        AZ::JsonSerializationResult::ResultCode nameResult = ContinueLoadingFromJsonObjectField(&property->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::name, context);
        if (nameResult.GetOutcome() == AZ::JsonSerializationResult::Outcomes::DefaultsUsed)
        {
            // This "id" key is for backward compatibility.
            result.Combine(ContinueLoadingFromJsonObjectField(&property->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::id, context));
        }
        else
        {
            result.Combine(nameResult);
        }

        result.Combine(ContinueLoadingFromJsonObjectField(&property->m_displayName, azrtti_typeid<AZStd::string>(), inputValue, Field::displayName, context));
        result.Combine(ContinueLoadingFromJsonObjectField(&property->m_description, azrtti_typeid<AZStd::string>(), inputValue, Field::description, context));
        result.Combine(ContinueLoadingFromJsonObjectField(&property->m_dataType, azrtti_typeid<PhysXMaterialPropertyDataType>(), inputValue, Field::type, context));

        switch (property->m_dataType)
        {
        case PhysXMaterialPropertyDataType::Bool:
            result.Combine(LoadNonNumericValues<bool>(property, false, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Int:
            result.Combine(LoadNumericValues<int32_t>(property, 0, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::UInt:
            result.Combine(LoadNumericValues<uint32_t>(property, 0u, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Float:
            result.Combine(LoadNumericValues<float>(property, 0.0f, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Vector2:
            result.Combine(LoadNonNumericValues<AZ::Vector2>(property, AZ::Vector2{0.0f, 0.0f}, inputValue, context));
            result.Combine(LoadVectorLabels(property, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Vector3:
            result.Combine(LoadNonNumericValues<AZ::Vector3>(property, AZ::Vector3{0.0f, 0.0f, 0.0f}, inputValue, context));
            result.Combine(LoadVectorLabels(property, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Vector4:
            result.Combine(LoadNonNumericValues<AZ::Vector4>(property, AZ::Vector4{0.0f, 0.0f, 0.0f, 0.0f}, inputValue, context));
            result.Combine(LoadVectorLabels(property, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Color:
            result.Combine(LoadNonNumericValues<AZ::Color>(property, AZ::Colors::White, inputValue, context));
            break;
        case PhysXMaterialPropertyDataType::Enum:
            result.Combine(LoadNonNumericValues<AZStd::string>(property, "", inputValue, context));
        default:
            result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Skipped));
            break;
        }

        if (inputValue.HasMember(Field::enumValues))
        {
            result.Combine(ContinueLoading(&property->m_enumValues, azrtti_typeid(property->m_enumValues), inputValue[Field::enumValues], context));
        }

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully loaded property definition.");
        }
        else
        {
            return context.Report(result, "Partially loaded property definition.");
        }
    }
        
    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::StoreNumericValues(
        rapidjson::Value& outputValue,
        const PhysXMaterialTypeSourceData::PropertyDefinition* property,
        const T& defaultValue,
        AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        JSR::ResultCode result(JSR::Tasks::WriteValue);

        if (property->m_value.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::defaultValue, &property->m_value.GetValue<T>(), &defaultValue, azrtti_typeid<T>(), context));
        }

        if (property->m_min.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::min, &property->m_min.GetValue<T>(), nullptr, azrtti_typeid<T>(), context));
        }

        if (property->m_max.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::max, &property->m_max.GetValue<T>(), nullptr, azrtti_typeid<T>(), context));
        }

        if (property->m_softMin.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::softMin, &property->m_softMin.GetValue<T>(), nullptr, azrtti_typeid<T>(), context));
        }

        if (property->m_softMax.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::softMax, &property->m_softMax.GetValue<T>(), nullptr, azrtti_typeid<T>(), context));
        }

        if (property->m_step.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::step, &property->m_step.GetValue<T>(), nullptr, azrtti_typeid<T>(), context));
        }

        return result;
    }

    template<typename T>
    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::StoreNonNumericValues(
        rapidjson::Value& outputValue,
        const PhysXMaterialTypeSourceData::PropertyDefinition* property,
        const T& defaultValue,
        AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        AZ::JsonSerializationResult::ResultCode result(JSR::Tasks::WriteValue);

        if (property->m_value.Is<T>())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::defaultValue, &property->m_value.GetValue<T>(), &defaultValue, azrtti_typeid<T>(), context));
        }

        return result;
    }


    AZ::JsonSerializationResult::Result JsonPhysXMaterialPropertySerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        [[maybe_unused]] const void* defaultValue, const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        AZ_Assert(azrtti_typeid<PhysXMaterialTypeSourceData::PropertyDefinition>() == valueTypeId,
            "Unable to serialize material property to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const PhysXMaterialTypeSourceData::PropertyDefinition* property = reinterpret_cast<const PhysXMaterialTypeSourceData::PropertyDefinition*>(inputValue);
        AZ_Assert(property, "Input value for JsonMaterialPropertySerializer can't be null.");
            
        JSR::ResultCode result(JSR::Tasks::WriteValue);

        outputValue.SetObject();

        const AZStd::string emptyString;
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::name, &property->m_name, &emptyString, azrtti_typeid<AZStd::string>(), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::displayName, &property->m_displayName, &emptyString, azrtti_typeid<AZStd::string>(), context));
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::description, &property->m_description, &emptyString, azrtti_typeid<AZStd::string>(), context));

        PhysXMaterialPropertyDataType defaultDataType = PhysXMaterialPropertyDataType::Invalid;
        result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::type, &property->m_dataType, &defaultDataType, azrtti_typeid(property->m_dataType), context));

        result.Combine(StoreVectorLabels(outputValue, property, context));

        switch (property->m_dataType)
        {
        case PhysXMaterialPropertyDataType::Bool:
            result.Combine(StoreNonNumericValues<bool>(outputValue, property, false, context));
            break;
        case PhysXMaterialPropertyDataType::Int:
            result.Combine(StoreNumericValues<int32_t>(outputValue, property, 0, context));
            break;
        case PhysXMaterialPropertyDataType::UInt:
            result.Combine(StoreNumericValues<uint32_t>(outputValue, property, 0u, context));
            break;
        case PhysXMaterialPropertyDataType::Float:
            result.Combine(StoreNumericValues<float>(outputValue, property, 0.0f, context));
            break;
        case PhysXMaterialPropertyDataType::Vector2:
            result.Combine(StoreNonNumericValues<AZ::Vector2>(outputValue, property, AZ::Vector2{0.0f, 0.0f}, context));
            break;
        case PhysXMaterialPropertyDataType::Vector3:
            result.Combine(StoreNonNumericValues<AZ::Vector3>(outputValue, property, AZ::Vector3{0.0f, 0.0f, 0.0f}, context));
            break;
        case PhysXMaterialPropertyDataType::Vector4:
            result.Combine(StoreNonNumericValues<AZ::Vector4>(outputValue, property, AZ::Vector4{0.0f, 0.0f, 0.0f, 0.0f}, context));
            break;
        case PhysXMaterialPropertyDataType::Color:
            result.Combine(StoreNonNumericValues<AZ::Color>(outputValue, property, AZ::Colors::White, context));
            break;
        case PhysXMaterialPropertyDataType::Enum:
            result.Combine(StoreNonNumericValues<AZStd::string>(outputValue, property, AZStd::string{""}, context));
            break;
        }

        // Enum list
        if (property->m_enumValues.size() > 0)
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::enumValues, &property->m_enumValues, nullptr, azrtti_typeid(property->m_enumValues), context));
        }

        if (result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed)
        {
            return context.Report(result, "Successfully stored property definition.");
        }
        else
        {
            return context.Report(result, "Partially stored property definition.");
        }
    }

    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::LoadVectorLabels(PhysXMaterialTypeSourceData::PropertyDefinition* intoProperty,
        const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        JSR::ResultCode result(JSR::Tasks::ReadField);

        if (inputValue.HasMember(Field::vectorLabels))
        {
            result.Combine(ContinueLoading(&intoProperty->m_vectorLabels, azrtti_typeid(intoProperty->m_vectorLabels), inputValue[Field::vectorLabels], context));
        }

        return result;
    }

    AZ::JsonSerializationResult::ResultCode JsonPhysXMaterialPropertySerializer::StoreVectorLabels(rapidjson::Value& outputValue,
        const PhysXMaterialTypeSourceData::PropertyDefinition* property, AZ::JsonSerializerContext& context)
    {
        AZStd::string emptyString;
        namespace JSR = AZ::JsonSerializationResult;
        using namespace JsonMaterialPropertySerializerInternal;

        AZ::JsonSerializationResult::ResultCode result(JSR::Tasks::WriteValue);

        if (!property->m_vectorLabels.empty())
        {
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::vectorLabels, &property->m_vectorLabels, nullptr, azrtti_typeid(property->m_vectorLabels), context));
        }

        return result;
    }
} // namespace PhysX
