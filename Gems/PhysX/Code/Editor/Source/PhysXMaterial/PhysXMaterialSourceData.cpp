/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/PhysXMaterial/PhysXMaterialSourceData.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <Editor/Source/PhysXMaterial/Serializer/PhysXMaterialPropertyValueSerializer.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialUtils.h>
//#include <Atom/RPI.Edit/Material/MaterialConverterBus.h>

#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
//#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
//#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>

#include <PhysXMaterial/MaterialAsset/PhysXMaterialAssetCreator.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
//#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace PhysX
{
    void PhysXMaterialSourceData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonPhysXMaterialPropertyValueSerializer>()->HandlesType<PhysXMaterialSourceData::Property>();
        }
        else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMaterialSourceData>()
                ->Version(2)
                ->Field("description", &PhysXMaterialSourceData::m_description)
                ->Field("materialType", &PhysXMaterialSourceData::m_materialType)
                ->Field("materialTypeVersion", &PhysXMaterialSourceData::m_materialTypeVersion)
                ->Field("parentMaterial", &PhysXMaterialSourceData::m_parentMaterial)
                ->Field("properties", &PhysXMaterialSourceData::m_properties)
                ;

            serializeContext->RegisterGenericType<PropertyMap>();
            serializeContext->RegisterGenericType<PropertyGroupMap>();

            serializeContext->Class<PhysXMaterialSourceData::Property>()
                ->Version(1)
                ;
        }
    }

    // Helper function for CreateMaterialAsset, for applying basic material property values
    template<typename T>
    void ApplyMaterialValues(PhysXMaterialAssetCreator& materialAssetCreator, const AZStd::map<AZ::Name, T>& values)
    {
        for (auto& entry : values)
        {
            const AZ::Name& propertyId = entry.first;
            materialAssetCreator.SetPropertyValue(propertyId, entry.second);
        }
    }
        
    AZ::Outcome<AZ::Data::Asset<PhysXMaterialAsset>> PhysXMaterialSourceData::CreateMaterialAsset(
        AZ::Data::AssetId assetId, AZStd::string_view materialSourceFilePath, PhysXMaterialAssetProcessingMode processingMode, bool elevateWarnings) const
    {
        PhysXMaterialAssetCreator materialAssetCreator;
        materialAssetCreator.SetElevateWarnings(elevateWarnings);

        if (m_materialType.empty())
        {
            AZ_Error("MaterialSourceData", false, "materialType was not specified");
            return AZ::Failure();
        }

        AZ::Outcome<AZ::Data::AssetId> materialTypeAssetId = AZ::RPI::AssetUtils::MakeAssetId(materialSourceFilePath, m_materialType, 0);
        if (!materialTypeAssetId)
        {
            return AZ::Failure();
        }

        AZ::Data::Asset<PhysXMaterialTypeAsset> materialTypeAsset;
            
        switch (processingMode)
        {
            case PhysXMaterialAssetProcessingMode::DeferredBake:
            {
                    // Don't load the material type data, just create a reference to it
                    materialTypeAsset = AZ::Data::Asset<PhysXMaterialTypeAsset>{ materialTypeAssetId.GetValue(), azrtti_typeid<PhysXMaterialTypeAsset>(), m_materialType };
                    break;
            }
            case PhysXMaterialAssetProcessingMode::PreBake:
            {
                // In this case we need to load the material type data in preparation for the material->Finalize() step below.
                auto materialTypeAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<PhysXMaterialTypeAsset>(materialTypeAssetId.GetValue());
                if (!materialTypeAssetOutcome)
                {
                    return AZ::Failure();
                }
                materialTypeAsset = materialTypeAssetOutcome.GetValue();
                break;
            }
            default:
            {
                AZ_Assert(false, "Unhandled MaterialAssetProcessingMode");
                return AZ::Failure();
            }
        }

        materialAssetCreator.Begin(assetId, materialTypeAsset, processingMode == PhysXMaterialAssetProcessingMode::PreBake);

        if (!m_parentMaterial.empty())
        {
            auto parentMaterialAsset = AZ::RPI::AssetUtils::LoadAsset<PhysXMaterialAsset>(materialSourceFilePath, m_parentMaterial);
            if (!parentMaterialAsset.IsSuccess())
            {
                return AZ::Failure();
            }

            // Make sure the parent material has the same material type
            {
                AZ::Data::AssetId parentsMaterialTypeId = parentMaterialAsset.GetValue()->GetMaterialTypeAsset().GetId();

                if (materialTypeAssetId.GetValue() != parentsMaterialTypeId)
                {
                    AZ_Error("MaterialSourceData", false, "This material and its parent material do not share the same material type.");
                    return AZ::Failure();
                }
            }

            // Inherit the parent's property values...
            switch (processingMode)
            {
                case PhysXMaterialAssetProcessingMode::DeferredBake:
                {
                    for (auto& property : parentMaterialAsset.GetValue()->GetRawPropertyValues())
                    {
                        materialAssetCreator.SetPropertyValue(property.first, property.second);
                    }

                    break;
                }
                case PhysXMaterialAssetProcessingMode::PreBake:
                {
                    // TODO: Try to bring back properties layout
                    /*
                    const PhysXMaterialPropertiesLayout* propertiesLayout = parentMaterialAsset.GetValue()->GetMaterialPropertiesLayout();

                    if (parentMaterialAsset.GetValue()->GetPropertyValues().size() != propertiesLayout->GetPropertyCount())
                    {
                        AZ_Assert(false, "The parent material should have been finalized with %zu properties but it has %zu. Something is out of sync.",
                            propertiesLayout->GetPropertyCount(), parentMaterialAsset.GetValue()->GetPropertyValues().size());
                        return AZ::Failure();
                    }

                    for (size_t propertyIndex = 0; propertyIndex < propertiesLayout->GetPropertyCount(); ++propertyIndex)
                    {
                        materialAssetCreator.SetPropertyValue(
                            propertiesLayout->GetPropertyDescriptor(PhysXMaterialPropertyIndex{propertyIndex})->GetName(),
                            parentMaterialAsset.GetValue()->GetPropertyValues()[propertyIndex]);
                    }
                    */
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unhandled MaterialAssetProcessingMode");
                    return AZ::Failure();
                }
            }
        }

        ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);

        AZ::Data::Asset<PhysXMaterialAsset> material;
        if (materialAssetCreator.End(material))
        {
            return AZ::Success(material);
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZ::Outcome<AZ::Data::Asset<PhysXMaterialAsset>> PhysXMaterialSourceData::CreateMaterialAssetFromSourceData(
        AZ::Data::AssetId assetId,
        AZStd::string_view materialSourceFilePath,
        bool elevateWarnings,
        AZStd::unordered_set<AZStd::string>* sourceDependencies) const
    {
        if (m_materialType.empty())
        {
            AZ_Error("MaterialSourceData", false, "materialType was not specified");
            return AZ::Failure();
        }

        const auto materialTypeSourcePath = AZ::RPI::AssetUtils::ResolvePathReference(materialSourceFilePath, m_materialType);
        const auto materialTypeAssetId = AZ::RPI::AssetUtils::MakeAssetId(materialTypeSourcePath, 0);
        if (!materialTypeAssetId.IsSuccess())
        {
            AZ_Error("MaterialSourceData", false, "Failed to create material type asset ID: '%s'.", materialTypeSourcePath.c_str());
            return AZ::Failure();
        }

        auto materialTypeLoadOutcome = PhysX::MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourcePath);
        if (!materialTypeLoadOutcome)
        {
            AZ_Error("MaterialSourceData", false, "Failed to load MaterialTypeSourceData: '%s'.", materialTypeSourcePath.c_str());
            return AZ::Failure();
        }

        PhysXMaterialTypeSourceData materialTypeSourceData = materialTypeLoadOutcome.TakeValue();

        const auto materialTypeAsset =
            materialTypeSourceData.CreateMaterialTypeAsset(materialTypeAssetId.GetValue(), materialTypeSourcePath, elevateWarnings);
        if (!materialTypeAsset.IsSuccess())
        {
            AZ_Error("MaterialSourceData", false, "Failed to create material type asset from source data: '%s'.", materialTypeSourcePath.c_str());
            return AZ::Failure();
        }

        // Track all of the material and material type assets loaded while trying to create a material asset from source data. This will
        // be used for evaluating circular dependencies and returned for external monitoring or other use.
        AZStd::unordered_set<AZStd::string> dependencies;
        dependencies.insert(materialSourceFilePath);
        dependencies.insert(materialTypeSourcePath);

        // Load and build a stack of MaterialSourceData from all of the parent materials in the hierarchy. Properties from the source
        // data will be applied in reverse to the asset creator.
        AZStd::vector<PhysXMaterialSourceData> parentSourceDataStack;

        AZStd::string parentSourceRelPath = m_parentMaterial;
        AZStd::string parentSourceAbsPath = AZ::RPI::AssetUtils::ResolvePathReference(materialSourceFilePath, parentSourceRelPath);
        while (!parentSourceRelPath.empty())
        {
            if (!dependencies.insert(parentSourceAbsPath).second)
            {
                AZ_Error("MaterialSourceData", false, "Detected circular dependency between materials: '%s' and '%s'.", materialSourceFilePath.data(), parentSourceAbsPath.c_str());
                return AZ::Failure();
            }

            PhysXMaterialSourceData parentSourceData;
            if (!AZ::RPI::JsonUtils::LoadObjectFromFile(parentSourceAbsPath, parentSourceData))
            {
                AZ_Error("MaterialSourceData", false, "Failed to load MaterialSourceData for parent material: '%s'.", parentSourceAbsPath.c_str());
                return AZ::Failure();
            }

            // Make sure that all materials in the hierarchy share the same material type
            const auto parentTypeAssetId = AZ::RPI::AssetUtils::MakeAssetId(parentSourceAbsPath, parentSourceData.m_materialType, 0);
            if (!parentTypeAssetId)
            {
                AZ_Error("MaterialSourceData", false, "Parent material asset ID wasn't found: '%s'.", parentSourceAbsPath.c_str());
                return AZ::Failure();
            }

            if (parentTypeAssetId.GetValue() != materialTypeAssetId.GetValue())
            {
                AZ_Error("MaterialSourceData", false, "This material and its parent material do not share the same material type.");
                return AZ::Failure();
            }

            // Get the location of the next parent material and push the source data onto the stack 
            parentSourceRelPath = parentSourceData.m_parentMaterial;
            parentSourceAbsPath = AZ::RPI::AssetUtils::ResolvePathReference(parentSourceAbsPath, parentSourceRelPath);
            parentSourceDataStack.emplace_back(AZStd::move(parentSourceData));
        }
            
        // Unlike CreateMaterialAsset(), we can always finalize the material here because we loaded created the MaterialTypeAsset from
        // the source .materialtype file, so the necessary data is always available.
        // (In case you are wondering why we don't use CreateMaterialAssetFromSourceData in MaterialBuilder: that would require a
        // source dependency between the .materialtype and .material file, which would cause all .material files to rebuild when you
        // edit the .materialtype; it's faster to not read the material type data at all ... until it's needed at runtime)
        const bool finalize = true;

        // Create the material asset from all the previously loaded source data 
        PhysXMaterialAssetCreator materialAssetCreator;
        materialAssetCreator.SetElevateWarnings(elevateWarnings);
        materialAssetCreator.Begin(assetId, materialTypeAsset.GetValue(), finalize);

        while (!parentSourceDataStack.empty())
        {
            parentSourceDataStack.back().ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);
            parentSourceDataStack.pop_back();
        }

        ApplyPropertiesToAssetCreator(materialAssetCreator, materialSourceFilePath);

        AZ::Data::Asset<PhysXMaterialAsset> material;
        if (materialAssetCreator.End(material))
        {
            if (sourceDependencies)
            {
                sourceDependencies->insert(dependencies.begin(), dependencies.end());
            }

            return AZ::Success(material);
        }

        return AZ::Failure();
    }

    void PhysXMaterialSourceData::ApplyPropertiesToAssetCreator(
        PhysXMaterialAssetCreator& materialAssetCreator, [[maybe_unused]] const AZStd::string_view& materialSourceFilePath) const
    {
        for (auto& group : m_properties)
        {
            for (auto& property : group.second)
            {
                AZ::RPI::MaterialPropertyId propertyId{ group.first, property.first };
                if (!property.second.m_value.IsValid())
                {
                    materialAssetCreator.ReportWarning("Source data for material property value is invalid.");
                }
                else
                {
                    materialAssetCreator.SetPropertyValue(propertyId, property.second.m_value);
                }
            }
        }
    }

} // namespace PhysX
