/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentSerializer.h>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorComponentSerializer, AZ::SystemAllocator);

    AZ::JsonSerializationResult::Result EditorComponentSerializer::Load(
        void* outputValue,
        [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
        [[maybe_unused]] const rapidjson::Value& inputValue,
        AZ::JsonDeserializerContext& context)
    {
        namespace JSR = AZ::JsonSerializationResult;

        AZ_Assert(
            azrtti_typeid<Components::EditorComponentBase>() == outputValueTypeId,
            "Unable to deserialize editor component from json because the provided type is %s.",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        Components::EditorComponentBase* editorComponent = reinterpret_cast<Components::EditorComponentBase*>(outputValue);
        AZ_Assert(editorComponent, "Output value for EditorComponentSerializer can't be null.");

        JSR::ResultCode result(JSR::Tasks::ReadField);

        AZStd::string_view message = result.GetProcessing() == JSR::Processing::Completed
            ? "Successfully loaded editor component information."
            : (result.GetProcessing() != JSR::Processing::Halted ? "Partially loaded editor component information."
                                                                 : "Failed to load editor component information.");

        return context.Report(result, message);
    }
} // namespace AzToolsFramework
