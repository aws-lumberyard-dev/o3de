
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysXMaterial/MaterialTypeAsset/PhysXMaterialTypeAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
//#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
//#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace PhysX
{
    const char* PhysXMaterialTypeAsset::DisplayName = "PhysXMaterialTypeAsset";
    const char* PhysXMaterialTypeAsset::Group = "PhysXMaterial";
    const char* PhysXMaterialTypeAsset::Extension = "azphysxmaterialtype";

    void PhysXMaterialTypeAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMaterialTypeAsset, AZ::Data::AssetData>()
                ->Version(5) // Material version update
                ->Field("Version", &PhysXMaterialTypeAsset::m_version)
                //->Field("MaterialPropertiesLayout", &PhysXMaterialTypeAsset::m_materialPropertiesLayout)
                ->Field("DefaultPropertyValues", &PhysXMaterialTypeAsset::m_propertyValues)
                ;
        }
    }

    PhysXMaterialTypeAsset::~PhysXMaterialTypeAsset()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::RPI::AssetInitBus::Handler::BusDisconnect();
    }

    /*
    const MaterialPropertiesLayout* MaterialTypeAsset::GetMaterialPropertiesLayout() const
    {
        return m_materialPropertiesLayout.get();
    }
    */

    AZStd::span<const PhysXMaterialPropertyValue> PhysXMaterialTypeAsset::GetDefaultPropertyValues() const
    {
        return m_propertyValues;
    }

    uint32_t PhysXMaterialTypeAsset::GetVersion() const
    {
        return m_version;
    }

    bool PhysXMaterialTypeAsset::ApplyPropertyRenames([[maybe_unused]] AZ::Name& propertyId) const
    {
        bool renamed = false;

        return renamed;
    }

    void PhysXMaterialTypeAsset::SetReady()
    {
        m_status = AssetStatus::Ready;

        // If this was created dynamically using MaterialTypeAssetCreator (which is what calls SetReady()),
        // we need to connect to the AssetBus for reloads.
        PostLoadInit();
    }

    bool PhysXMaterialTypeAsset::PostLoadInit()
    {
        AZ::RPI::AssetInitBus::Handler::BusDisconnect();

        return true;
    }

    template<typename AssetDataT>
    void TryReplaceAsset(AZ::Data::Asset<AssetDataT>& assetToReplace, const AZ::Data::Asset<AZ::Data::AssetData>& newAsset)
    {
        if (assetToReplace.GetId() == newAsset.GetId())
        {
            assetToReplace = newAsset;
        }
    }
        
    void PhysXMaterialTypeAsset::ReinitializeAsset(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Notify interested parties that this MaterialTypeAsset is changed and may require other data to reinitialize as well
        //MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialTypeAssetReinitialized, Data::Asset<MaterialTypeAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
    }

    void PhysXMaterialTypeAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        //ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialTypeAsset::OnAssetReloaded %s", this, asset.GetHint().c_str());
        ReinitializeAsset(asset);
    }
        
    void PhysXMaterialTypeAsset::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Regarding why we listen to both OnAssetReloaded and OnAssetReady, see explanation in ShaderAsset::OnAssetReady.
        //ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialTypeAsset::OnAssetReady %s", this, asset.GetHint().c_str());
        ReinitializeAsset(asset);
    }

    AZ::Data::AssetHandler::LoadResult PhysXMaterialTypeAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            asset.GetAs<PhysXMaterialTypeAsset>()->AZ::RPI::AssetInitBus::Handler::BusConnect();
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }
}
