/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/PhysXMaterial/PhysXMaterialUtils.h>
//#include <Atom/RPI.Edit/Common/AssetUtils.h>
//#include <Atom/RPI.Reflect/Image/ImageAsset.h>
//#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
//#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
//#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Settings/SettingsRegistry.h>

#include <AzCore/std/string/string.h>

namespace PhysX
{
    namespace MaterialUtils
    {
        AZ::Outcome<PhysXMaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, const rapidjson::Value* document)
        {
            AZ::Outcome<rapidjson::Document, AZStd::string> loadOutcome;
            if (document == nullptr)
            {
                loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(filePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                if (!loadOutcome.IsSuccess())
                {
                    AZ_Error("AZ::RPI::JsonUtils", false, "%s", loadOutcome.GetError().c_str());
                    return AZ::Failure();
                }

                document = &loadOutcome.GetValue();
            }

            PhysXMaterialTypeSourceData materialType;

            AZ::JsonDeserializerSettings settings;

            AZ::RPI::JsonReportingHelper reportingHelper;
            reportingHelper.Attach(settings);

            // This is required by some custom material serializers to support relative path references.
            AZ::RPI::JsonFileLoadContext fileLoadContext;
            fileLoadContext.PushFilePath(filePath);
            settings.m_metadata.Add(fileLoadContext);

            AZ::JsonSerialization::Load(materialType, *document, settings);
            materialType.ConvertToNewDataFormat();

            if (reportingHelper.ErrorsReported())
            {
                return AZ::Failure();
            }
            else
            {
                return AZ::Success(AZStd::move(materialType));
            }
        }

        void CheckForUnrecognizedJsonFields(
            const AZStd::string_view* acceptedFieldNames,
            uint32_t acceptedFieldNameCount,
            const rapidjson::Value& object, AZ::JsonDeserializerContext& context,
            AZ::JsonSerializationResult::ResultCode &result)
        {
            for (auto iter = object.MemberBegin(); iter != object.MemberEnd(); ++iter)
            {
                bool matched = false;

                for (uint32_t i = 0; i < acceptedFieldNameCount; ++i)
                {
                    if (iter->name.GetString() == acceptedFieldNames[i])
                    {
                        matched = true;
                        break;
                    }
                }

                if (!matched)
                {
                    AZ::ScopedContextPath subPath{context, iter->name.GetString()};
                    result.Combine(context.Report(AZ::JsonSerializationResult::Tasks::ReadField, AZ::JsonSerializationResult::Outcomes::Skipped, "Skipping unrecognized field"));
                }
            }
        }
            
        bool BuildersShouldFinalizeMaterialAssets()
        {
            // We default to the faster workflow for developers. Enable this registry setting when releasing the
            // game for faster load times and obfuscation of material assets.
            bool shouldFinalize = false;

            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(shouldFinalize, "/O3DE/PhysX/MaterialBuilder/FinalizeMaterialAssets");
            }

            return shouldFinalize;
        }

        bool ConvertToExportFormat(
            [[maybe_unused]] const AZStd::string& exportPath,
            [[maybe_unused]] const AZ::Name& propertyId,
            const PhysXMaterialTypeSourceData::PropertyDefinition& propertyDefinition,
            PhysXMaterialPropertyValue& propertyValue)
        {
            if (propertyDefinition.m_dataType == PhysXMaterialPropertyDataType::Enum && propertyValue.Is<uint32_t>())
            {
                const uint32_t index = propertyValue.GetValue<uint32_t>();
                if (index >= propertyDefinition.m_enumValues.size())
                {
                    AZ_Error("AtomToolsFramework", false, "Invalid value for material enum property: '%s'.", propertyId.GetCStr());
                    return false;
                }

                propertyValue = propertyDefinition.m_enumValues[index];
                return true;
            }

            return true;
        }
    }
}
