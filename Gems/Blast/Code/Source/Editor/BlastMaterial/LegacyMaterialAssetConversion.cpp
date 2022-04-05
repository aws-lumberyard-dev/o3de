/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <BlastMaterial/MaterialConfiguration.h>
#include <BlastMaterial/MaterialAsset.h>

namespace Blast
{
    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs);

    AZ_CONSOLEFREEFUNC("blast_convertMaterialLibrariesIntoIndividualMaterials", ConvertMaterialLibrariesIntoIndividualMaterials, AZ::ConsoleFunctorFlags::Null,
        "Finds legacy blast material library assets in the project and generates new individual blast material assets. Original library assets will be deleted.");

    // DEPRECATED
    // A single BlastMaterial entry in the material library
    // BlastMaterialLibraryAsset holds a collection of BlastMaterialFromAssetConfiguration instances.
    class BlastMaterialFromAssetConfiguration
    {
    public:
        AZ_TYPE_INFO(BlastMaterialFromAssetConfiguration, "{E380E174-BCA3-4BBB-AA39-8FAD39005B12}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastMaterialFromAssetConfiguration>()
                    ->Version(1)
                    ->Field("Configuration", &BlastMaterialFromAssetConfiguration::m_configuration)
                    ->Field("UID", &BlastMaterialFromAssetConfiguration::m_id);
            }
        }

        MaterialConfiguration m_configuration;
        BlastMaterialId m_id;
    };

    // DEPRECATED
    // An asset that holds a list of materials.
    class BlastMaterialLibraryAsset : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialLibraryAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(BlastMaterialLibraryAsset, "{55F38C86-0767-4E7F-830A-A4BF624BE4DA}", AZ::Data::AssetData);

        BlastMaterialLibraryAsset() = default;
        virtual ~BlastMaterialLibraryAsset() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BlastMaterialLibraryAsset>()
                    ->Version(1)
                    ->Field("Properties", &BlastMaterialLibraryAsset::m_materialLibrary);
            }
        }

        AZStd::vector<BlastMaterialFromAssetConfiguration> m_materialLibrary;
    };

    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context)
    {
        BlastMaterialFromAssetConfiguration::Reflect(context);
        BlastMaterialLibraryAsset::Reflect(context);
    }

    void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
    {
        auto* materialAssetHandler = AZ::Data::AssetManager::Instance().GetHandler(MaterialAsset::RTTI_Type());
        if (!materialAssetHandler)
        {
            AZ_Error("Blast", false, "Unable to find blast MaterialAsset handler.");
            return;
        }

        struct BlastLibrary
        {
            AZStd::vector<BlastMaterialFromAssetConfiguration> m_materialAssetConfigurations;
            AZStd::string m_sourceFile; // Path to source file. Used during conversion to new material asset.
        };

        // Collect all BlastMaterialLibraryAsset
        AZStd::vector<BlastLibrary> materialLibraries;
        {
            // Unregister the new MaterialAsset handler for .blastmaterial files
            AZ::Data::AssetManager::Instance().UnregisterHandler(materialAssetHandler);

            // Create asset handler for BlastMaterialLibraryAsset for .blastmaterial files
            auto materialLibraryAsset = AZStd::make_unique<AzFramework::GenericAssetHandler<BlastMaterialLibraryAsset>>(
                "Blast Material", "Blast Material", "blastmaterial");
            materialLibraryAsset->Register();

            // For each .blastmaterial file
                // Is it valid? Meaning it's a BlastMaterialLibraryAsset
                    // Read and add it to vector.

            // automatically register all layer categories assets
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
                nullptr,
                [&materialLibraries](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo) {
                    if (assetInfo.m_assetType == BlastMaterialLibraryAsset::RTTI_Type())
                    {
                        AZ::Data::Asset<BlastMaterialLibraryAsset> materialLibrary =
                            AZ::Data::AssetManager::Instance().GetAsset<BlastMaterialLibraryAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                        materialLibrary.QueueLoad();
                        materialLibrary.BlockUntilLoadComplete();

                        BlastLibrary blastLibrary;
                        blastLibrary.m_materialAssetConfigurations = materialLibrary->m_materialLibrary;
                        blastLibrary.m_sourceFile = assetInfo.m_relativePath;

                        materialLibraries.push_back(AZStd::move(blastLibrary));
                    }
                },
                nullptr);

            materialLibraryAsset->Unregister();
            materialLibraryAsset.reset();

            // Register back the new MaterialAsset handler for .blastmaterial files
            AZ::Data::AssetManager::Instance().UnregisterHandler(materialAssetHandler);
        }

        AZ_Warning("Blast", false, "Material Libraries:");
        for (const auto& materialLibrary : materialLibraries)
        {
            AZ_Warning("Blast", false, "- Path: %s", materialLibrary.m_sourceFile.c_str());

            for (const auto& materialAssetConfiguration : materialLibrary.m_materialAssetConfigurations)
            {
                AZ_Warning("Blast", false, "    + BlastId: %d Health: %0.3f",
                    materialAssetConfiguration.m_id.m_id.ToString<AZStd::string>().c_str(),
                    materialAssetConfiguration.m_configuration.m_health);
            }
        }
    }
} // namespace Blast
