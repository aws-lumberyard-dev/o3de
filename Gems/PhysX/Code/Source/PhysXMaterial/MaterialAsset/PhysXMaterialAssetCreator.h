/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <PhysXMaterial/PhysXMaterialAsset.h>

namespace PhysX
{
    //! Use a MaterialAssetCreator to create and configure a new MaterialAsset.
    //!
    //! There are two options for how to create the MaterialAsset, whether it should be finalized now or deferred.
    //!  - Finalized now: This requires the MaterialTypeAsset to be fully populated so it can read the property layout.
    //!  - Deferred finalize: This only requires the MaterialTypeAsset to have a valid AssetId; the data inside will not be used. MaterialAsset::Finalize()
    //!                       will need to be called later when the final MaterialTypeAsset is available, presumably after loading the MaterialAsset at runtime.
    class MaterialAssetCreator  
        : public AZ::RPI::AssetCreator<PhysXMaterialAsset>
    {
    public:
        friend class PhysXMaterialSourceData;

        void Begin(const AZ::Data::AssetId& assetId, const AZ::Data::Asset<PhysXMaterialTypeAsset>& materialType, bool shouldFinalize);
        bool End(AZ::Data::Asset<PhysXMaterialAsset>& result);

        void SetMaterialTypeVersion(uint32_t version);
            
        void SetPropertyValue(const AZ::Name& name, const PhysXMaterialPropertyValue& value);

    private:
        bool m_shouldFinalize = false;
    };
} // namespace PhysX
