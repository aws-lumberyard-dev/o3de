/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/ComponentSerializer.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Sfmt.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(ComponentSerializer, SystemAllocator);

    JsonSerializationResult::Result ComponentSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const Uuid& outputValueTypeId,
        [[maybe_unused]] const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<Component>() == outputValueTypeId,
            "Unable to deserialize component from json because the provided type is %s.",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        Component* component = reinterpret_cast<Component*>(outputValue);
        AZ_Assert(component, "Output value for ComponentSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        AZStd::string_view message = result.GetProcessing() == JSR::Processing::Completed
            ? "Successfully loaded editor component information."
            : (result.GetProcessing() != JSR::Processing::Halted ? "Partially loaded component information."
                                                                 : "Failed to load component information.");

        return context.Report(result, message);
    }

    JsonSerializationResult::ResultCode ComponentSerializer::ContinueLoadingFromJsonObjectField(
        void* object,
        const Uuid& typeId,
        const rapidjson::Value& value,
        rapidjson::Value::StringRefType memberName,
        JsonDeserializerContext& context,
        ContinuationFlags flags)
    {
        using namespace JsonSerializationResult;

        if (value.IsObject())
        {
            if (strcmp(memberName.s, "Id") == 0)
            {
                auto iter = value.FindMember(memberName);
                if (iter != value.MemberEnd())
                {
                    ScopedContextPath subPath{ context, memberName.s };
                    return ContinueLoading(object, typeId, iter->value, context, flags);
                }
                else
                {
                    AZ::u64* id = reinterpret_cast<AZ::u64*>(object);
                    *id = Sfmt::GetInstance().Rand64();
                }
            }
        }
        else
        {
            return context.Report(Tasks::ReadField, Outcomes::Unsupported, "Value is not an object");
        }

        return ResultCode(Tasks::ReadField, Outcomes::DefaultsUsed);
    }

    AZ::JsonSerializationResult::Result ComponentSerializer::Store(
        [[maybe_unused]]rapidjson::Value& outputValue,
        const void* inputValue,
        [[maybe_unused]] const void* defaultValue,
        const AZ::Uuid& valueTypeId,
        AZ::JsonSerializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<Component>() == valueTypeId,
            "Unable to Serialize component because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());

        const Component* instance = reinterpret_cast<const Component*>(inputValue);
        AZ_Assert(instance, "Input value for ComponentSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::WriteValue);

        {
            AZ::ScopedContextPath subPathSource(context, "m_id");

            result = JSR::ResultCode(JSR::Tasks::WriteValue, JSR::Outcomes::Success);
        }
        return context.Report(
            result,
            result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Component information."
                                                                 : "Failed to store Component information.");
    }
} // namespace
