/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/any.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/std/containers/span.h>

#include <Atom/RPI.Public/AssetInitBus.h>
//#include <Atom/RPI.Reflect/Base.h>
//#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>

namespace PhysX
{
    class PhysXMaterialTypeAssetHandler;

    //! MaterialTypeAsset defines the property layout and general behavior for 
    //! a type of material. It serves as the foundation for MaterialAssets,
    //! which can be used to render meshes at runtime.
    //! 
    //! Use a MaterialTypeAssetCreator to create a MaterialTypeAsset.
    class PhysXMaterialTypeAsset
        : public AZ::Data::AssetData
        , public AZ::Data::AssetBus::MultiHandler
        , public AZ::RPI::AssetInitBus::Handler
    {
        friend class PhysXMaterialTypeAssetCreator;
        friend class PhysXMaterialTypeAssetHandler;

    public:
        AZ_RTTI(PhysXMaterialTypeAsset, "{3B986416-5433-48F3-B3F8-C7F51D2FEDCE}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(PhysXMaterialTypeAsset, AZ::SystemAllocator, 0);

        static const char* DisplayName;
        static const char* Group;
        static const char* Extension;

        static constexpr uint32_t InvalidShaderIndex = static_cast<uint32_t>(-1);

        static void Reflect(AZ::ReflectContext* context);

        virtual ~PhysXMaterialTypeAsset();

        //! Returns the list of values for all properties in this material.
        //! The entries in this list align with the entries in the MaterialPropertiesLayout. Each AZStd::any is guaranteed 
        //! to have a value of type that matches the corresponding MaterialPropertyDescriptor.
        //! For images, the value will be of type ImageBinding.
        AZStd::span<const PhysXMaterialPropertyValue> GetDefaultPropertyValues() const;

        //! Returns the version of the MaterialTypeAsset.
        uint32_t GetVersion() const;

        //! Possibly renames @propertyId based on the material version update steps.
        //! @return true if the property was renamed
        bool ApplyPropertyRenames(AZ::Name& propertyId) const;

    private:
        bool PostLoadInit() override;

        //! Called by asset creators to assign the asset to a ready state.
        void SetReady();

        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        //! Replaces the appropriate asset members when a reload occurs
        void ReinitializeAsset(AZ::Data::Asset<AZ::Data::AssetData> asset);

        //! Holds values for each material property, used to initialize Material instances.
        //! This is indexed by MaterialPropertyIndex and aligns with entries in m_materialPropertiesLayout.
        AZStd::vector<PhysXMaterialPropertyValue> m_propertyValues;

        //! The version of this MaterialTypeAsset. If the version is greater than 1, actions performed
        //! to update this MaterialTypeAsset will be in m_materialVersionUpdateMap
        uint32_t m_version = 1;
    };

    class PhysXMaterialTypeAssetHandler : public AZ::RPI::AssetHandler<PhysXMaterialTypeAsset>
    {
        using Base = AZ::RPI::AssetHandler<PhysXMaterialTypeAsset>;
    public:
        AZ_RTTI(PhysXMaterialTypeAssetHandler, "{0636CF07-9240-46C7-9B34-4709E0AA0D3D}", Base);

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
    };
}
