/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Editor/Source/PhysXMaterial/MaterialTypeAsset/MaterialTypeAsset.h>
#include <AzCore/std/containers/span.h>

namespace PhysX
{
    //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
    //! The MaterialAsset will be based on a MaterialTypeAsset, which provides the 
    //! necessary data to define the layout and behavior of the material. The 
    //! MaterialAsset itself only provides property values.
    //! The MaterialAsset may optionally inherit from another 'parent' MaterialAsset,
    //! which provides the MaterialTypeAsset and default property values.
    class PhysXMaterialTypeAssetCreator
        : public AZ::RHI::AssetCreator<PhysXMaterialTypeAsset>
    {
    public:
        //! Begin creating a MaterialTypeAsset
        void Begin(const AZ::Data::AssetId& assetId);

        //! Sets the version of the MaterialTypeAsset
        void SetVersion(uint32_t version);

        //! Starts creating a material property.
        //! Note that EndMaterialProperty() must be called before calling SetMaterialPropertyValue(). Similarly,
        //! the property will not appear in GetMaterialPropertiesLayout() until EndMaterialProperty() is called.
        void BeginMaterialProperty(const AZ::Name& materialPropertyName, PhysXMaterialPropertyDataType dataType);

        //! Store the enum names if a property is an enum type.
        void SetMaterialPropertyEnumNames(const AZStd::vector<AZStd::string>& enumNames);

        //! Finishes creating a material property.
        void EndMaterialProperty();

        //! Sets a property value using data in AZStd::variant-based MaterialPropertyValue. The contained data must match
        //! the data type of the property. For type Image, the value must be a Data::Asset<ImageAsset>.
        void SetPropertyValue(const Name& name, const PhysXMaterialPropertyValue& value);

        bool End(AZ::Data::Asset<PhysXMaterialTypeAsset>& result);

    private:
        bool PropertyCheck(AZ::TypeId typeId, const Name& name);
            
        bool ValidateMaterialVersion();
        bool ValidateBeginMaterialProperty();
        bool ValidateEndMaterialProperty();
    };
} // namespace PhysX
