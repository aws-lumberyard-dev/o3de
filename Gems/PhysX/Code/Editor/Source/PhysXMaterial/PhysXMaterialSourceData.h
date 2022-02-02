/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Asset/AssetCommon.h>

#include <PhysXMaterial/PhysXMaterialPropertyValue.h>

namespace PhysX
{
    class PhysXMaterialAsset;
    class PhysXMaterialAssetCreator;

    enum class PhysXMaterialAssetProcessingMode
    {
        PreBake,      //!< all material asset processing is done in the Asset Processor, producing a finalized material asset
        DeferredBake  //!< some material asset processing is deferred, and the material asset is finalized at runtime after loading
    };

    //! This is a simple data structure for serializing in/out material source files.
    class PhysXMaterialSourceData final
    {
    public:
        AZ_TYPE_INFO(PhysX::PhysXMaterialSourceData, "{FA8F0ED4-6BEA-4327-9294-C519D6766A79}");

        static constexpr const char Extension[] = "physxmaterial";

        static void Reflect(AZ::ReflectContext* context);

        PhysXMaterialSourceData() = default;
            
        AZStd::string m_description;
            
        AZStd::string m_materialType; //!< The material type that defines the interface and behavior of the material
            
        AZStd::string m_parentMaterial; //!< The immediate parent of this material

        uint32_t m_materialTypeVersion = 0; //!< The version of the material type that was used to configure this material

        struct Property
        {
            AZ_TYPE_INFO(PhysX::PhysXMaterialSourceData::Property, "{71EACA5F-003D-4BBD-A119-26A545234FE6}");

            PhysXMaterialPropertyValue m_value;
        };

        using PropertyMap = AZStd::map<AZStd::string, Property>;
        using PropertyGroupMap = AZStd::map<AZStd::string, PropertyMap>;

        PropertyGroupMap m_properties;

        enum class ApplyVersionUpdatesResult
        {
            Failed,
            NoUpdates,
            UpdatesApplied
        };

        //! Creates a PhysXMaterialAsset from the PhysXMaterialSourceData content.
        //! @param assetId ID for the MaterialAsset
        //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for
        //! resolving file-relative paths.
        //! @param processingMode Indicates whether to finalize the material asset using data from the MaterialTypeAsset.
        //! @param elevateWarnings Indicates whether to treat warnings as errors
        AZ::Outcome<AZ::Data::Asset<PhysXMaterialAsset>> CreateMaterialAsset(
            AZ::Data::AssetId assetId,
            AZStd::string_view materialSourceFilePath,
            PhysXMaterialAssetProcessingMode processingMode,
            bool elevateWarnings = true) const;

        //! Creates a PhysXMaterialAsset from the PhysXMaterialSourceData content.
        //! @param assetId ID for the MaterialAsset
        //! @param materialSourceFilePath Indicates the path of the .material file that the MaterialSourceData represents. Used for
        //! resolving file-relative paths.
        //! @param elevateWarnings Indicates whether to treat warnings as errors
        //! @param sourceDependencies if not null, will be populated with a set of all of the loaded material and material type paths
        AZ::Outcome<AZ::Data::Asset<PhysXMaterialAsset>> CreateMaterialAssetFromSourceData(
            AZ::Data::AssetId assetId,
            AZStd::string_view materialSourceFilePath = "",
            bool elevateWarnings = true,
            AZStd::unordered_set<AZStd::string>* sourceDependencies = nullptr) const;

    private:
        void ApplyPropertiesToAssetCreator(
            PhysXMaterialAssetCreator& materialAssetCreator, const AZStd::string_view& materialSourceFilePath) const;
    };
} // namespace PhysX
