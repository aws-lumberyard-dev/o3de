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

#include <BlastMaterial/LegacyMaterial.h>
#include <BlastMaterial/MaterialConfiguration.h>

namespace Blast
{
    void BlastMaterialId::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Blast::BlastMaterialId>()
                ->Version(1)
                ->Field("BlastMaterialId", &Blast::BlastMaterialId::m_id);
        }
    }

    BlastMaterialId BlastMaterialId::Create()
    {
        BlastMaterialId id;
        id.m_id = AZ::Uuid::Create();
        return id;
    }

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
        BlastMaterialId::Reflect(context);
        BlastMaterialFromAssetConfiguration::Reflect(context);
        BlastMaterialLibraryAsset::Reflect(context);
    }

    namespace Internal
    {
        void ConvertMaterialLibrariesIntoIndividualMaterials(const AZ::ConsoleCommandContainer& commandArgs);

        AZ_CONSOLEFREEFUNC("blast_convertMaterialLibrariesIntoIndividualMaterials", ConvertMaterialLibrariesIntoIndividualMaterials, AZ::ConsoleFunctorFlags::Null,
            "Finds legacy blast material library assets in the project and generates new individual blast material assets. Original library assets will be deleted.");

        void ConvertMaterialLibrariesIntoIndividualMaterials([[maybe_unused]] const AZ::ConsoleCommandContainer& commandArgs)
        {
            AZ_Warning("Blast", false, "Do amazing conversion!");
        }

    } // namespace Internal
} // namespace Blast
