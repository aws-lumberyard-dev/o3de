/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXMaterial/MaterialAsset/PhysXMaterialAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
//#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
//#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TickBus.h>

namespace PhysX
{
    const char* PhysXMaterialAsset::s_debugTraceName = "PhysXMaterialAsset";

    const char* PhysXMaterialAsset::DisplayName = "PhysXMaterialAsset";
    const char* PhysXMaterialAsset::Group = "PhysXMaterial";
    const char* PhysXMaterialAsset::Extension = "azphysxmaterial";

    void PhysXMaterialAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMaterialAsset, AZ::Data::AssetData>()
                ->Version(14) // added m_rawPropertyValues
                ->Field("materialTypeAsset", &PhysXMaterialAsset::m_materialTypeAsset)
                ->Field("materialTypeVersion", &PhysXMaterialAsset::m_materialTypeVersion)
                ->Field("propertyValues", &PhysXMaterialAsset::m_propertyValues)
                ->Field("rawPropertyValues", &PhysXMaterialAsset::m_rawPropertyValues)
                ->Field("finalized", &PhysXMaterialAsset::m_wasPreFinalized)
                ;
        }
    }

    PhysXMaterialAsset::PhysXMaterialAsset()
    {
    }

    PhysXMaterialAsset::~PhysXMaterialAsset()
    {
        //MaterialReloadNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::RPI::AssetInitBus::Handler::BusDisconnect();
    }

    const AZ::Data::Asset<PhysXMaterialTypeAsset>& PhysXMaterialAsset::GetMaterialTypeAsset() const
    {
        return m_materialTypeAsset;
    }

    /*const PhysXMaterialPropertiesLayout* PhysXMaterialAsset::GetMaterialPropertiesLayout() const
    {
        return m_materialTypeAsset->GetMaterialPropertiesLayout();
    }*/
        
    bool PhysXMaterialAsset::WasPreFinalized() const
    {
        return m_wasPreFinalized;
    }

    //! Attempts to convert a numeric MaterialPropertyValue to another numeric type @T,
    //! since MaterialPropertyValue itself does not support any kind of casting.
    //! If the original MaterialPropertyValue is not a numeric type, the original value is returned.
    template<typename T>
    PhysXMaterialPropertyValue CastNumericMaterialPropertyValue(const PhysXMaterialPropertyValue& value)
    {
        AZ::TypeId typeId = value.GetTypeId();

        if (typeId == azrtti_typeid<bool>())
        {
            return aznumeric_cast<T>(value.GetValue<bool>());
        }
        else if (typeId == azrtti_typeid<int32_t>())
        {
            return aznumeric_cast<T>(value.GetValue<int32_t>());
        }
        else if (typeId == azrtti_typeid<uint32_t>())
        {
            return aznumeric_cast<T>(value.GetValue<uint32_t>());
        }
        else if (typeId == azrtti_typeid<float>())
        {
            return aznumeric_cast<T>(value.GetValue<float>());
        }
        else
        {
            return value;
        }
    }
        
    //! Attempts to convert an AZ::Vector[2-4] MaterialPropertyValue to another AZ::Vector[2-4] type @T.
    //! Any extra elements will be dropped or set to 0.0 as needed.
    //! If the original MaterialPropertyValue is not a Vector type, the original value is returned.
    template<typename VectorT>
    PhysXMaterialPropertyValue CastVectorMaterialPropertyValue(const PhysXMaterialPropertyValue& value)
    {
        float values[4] = {};

        AZ::TypeId typeId = value.GetTypeId();
        if (typeId == azrtti_typeid<AZ::Vector2>())
        {
            value.GetValue<AZ::Vector2>().StoreToFloat2(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector3>())
        {
            value.GetValue<AZ::Vector3>().StoreToFloat3(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector4>())
        {
            value.GetValue<AZ::Vector4>().StoreToFloat4(values);
        }
        else
        {
            return value;
        }

        typeId = azrtti_typeid<VectorT>();
        if (typeId == azrtti_typeid<AZ::Vector2>())
        {
            return AZ::Vector2::CreateFromFloat2(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector3>())
        {
            return AZ::Vector3::CreateFromFloat3(values);
        }
        else if (typeId == azrtti_typeid<AZ::Vector4>())
        {
            return AZ::Vector4::CreateFromFloat4(values);
        }
        else
        {
            return value;
        }
    }
        
    void PhysXMaterialAsset::Finalize(AZStd::function<void(const char*)> reportWarning, AZStd::function<void(const char*)> reportError)
    {
        if (m_wasPreFinalized)
        {
            m_isFinalized = true;
        }

        if (m_isFinalized)
        {
            return;
        }

        if (!reportWarning)
        {
            reportWarning = []([[maybe_unused]] const char* message)
            {
                AZ_Warning(s_debugTraceName, false, "%s", message);
            };
        }
            
        if (!reportError)
        {
            reportError = []([[maybe_unused]] const char* message)
            {
                AZ_Error(s_debugTraceName, false, "%s", message);
            };
        }

        /*const uint32_t materialTypeVersion = m_materialTypeAsset->GetVersion();
        if (m_materialTypeVersion < materialTypeVersion)
        {
            // It is possible that the material type has had some properties renamed or otherwise updated. If that's the case,
            // and this material is still referencing the old property layout, we need to apply any auto updates to rename those
            // properties before using them to realign the property values.
            ApplyVersionUpdates();
        }*/
            
        //const PhysXMaterialPropertiesLayout* propertyLayout = GetMaterialPropertiesLayout();

        AZStd::vector<PhysXMaterialPropertyValue> finalizedPropertyValues(m_materialTypeAsset->GetDefaultPropertyValues().begin(), m_materialTypeAsset->GetDefaultPropertyValues().end());

        // TODO: Consider briging back layout?
        /*
        for (const auto& [name, value] : m_rawPropertyValues)
        {
            const PhysXMaterialPropertyIndex propertyIndex = propertyLayout->FindPropertyIndex(name);
            if (propertyIndex.IsValid())
            {
                const PhysXMaterialPropertyDescriptor* propertyDescriptor = propertyLayout->GetPropertyDescriptor(propertyIndex);

                if (value.Is<AZStd::string>() && propertyDescriptor->GetDataType() == MaterialPropertyDataType::Enum)
                {
                    AZ::Name enumName = AZ::Name(value.GetValue<AZStd::string>());
                    uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                    if (enumValue == PhysXMaterialPropertyDescriptor::InvalidEnumValue)
                    {
                        reportWarning(AZStd::string::format("Material property name \"%s\" has invalid enum value \"%s\".", name.GetCStr(), enumName.GetCStr()).c_str());
                    }
                    else
                    {
                        finalizedPropertyValues[propertyIndex.GetIndex()] = enumValue;
                    }
                }
                else
                {
                    // The material asset could be finalized sometime after the original JSON is loaded, and the material type might not have been available
                    // at that time, so the data type would not be known for each property. So each raw property's type was based on what appeared in the JSON
                    // and here we have the first opportunity to resolve that value with the actual type. For example, a float property could have been specified in
                    // the JSON as 7 instead of 7.0, which is valid. Similarly, a Color and a Vector3 can both be specified as "[0.0,0.0,0.0]" in the JSON file.

                    PhysXMaterialPropertyValue finalValue = value;

                    switch (propertyDescriptor->GetDataType())
                    {
                    case PhysXMaterialPropertyDataType::Bool:
                        finalValue = CastNumericMaterialPropertyValue<bool>(value);
                        break;
                    case PhysXMaterialPropertyDataType::Int:
                        finalValue = CastNumericMaterialPropertyValue<int32_t>(value);
                        break;
                    case PhysXMaterialPropertyDataType::UInt:
                        finalValue = CastNumericMaterialPropertyValue<uint32_t>(value);
                        break;
                    case PhysXMaterialPropertyDataType::Float:
                        finalValue = CastNumericMaterialPropertyValue<float>(value);
                        break;
                    case PhysXMaterialPropertyDataType::Color:
                        if (value.GetTypeId() == azrtti_typeid<AZ::Vector3>())
                        {
                            finalValue = AZ::Color::CreateFromVector3(value.GetValue<Vector3>());
                        }
                        else if (value.GetTypeId() == azrtti_typeid<AZ::Vector4>())
                        {
                            AZ::Vector4 vector4 = value.GetValue<AZ@@Vector4>();
                            finalValue = AZ::Color::CreateFromVector3AndFloat(vector4.GetAsVector3(), vector4.GetW());
                        }
                        break;
                    case PhysXMaterialPropertyDataType::Vector2:
                        finalValue = CastVectorMaterialPropertyValue<AZ::Vector2>(value);
                        break;
                    case PhysXMaterialPropertyDataType::Vector3:
                        if (value.GetTypeId() == azrtti_typeid<AZ::Color>())
                        {
                            finalValue = value.GetValue<AZ::Color>().GetAsVector3();
                        }
                        else
                        {
                            finalValue = CastVectorMaterialPropertyValue<AZ::Vector3>(value);
                        }
                        break;
                    case PhysXMaterialPropertyDataType::Vector4:
                        if (value.GetTypeId() == azrtti_typeid<AZ::Color>())
                        {
                            finalValue = value.GetValue<AZ::Color>().GetAsVector4();
                        }
                        else
                        {
                            finalValue = CastVectorMaterialPropertyValue<AZ::Vector4>(value);
                        }
                        break;
                    }

                    if (ValidateMaterialPropertyDataType(finalValue.GetTypeId(), name, propertyDescriptor, reportError))
                    {
                        finalizedPropertyValues[propertyIndex.GetIndex()] = finalValue;
                    }
                }
            }
            else
            {
                reportWarning(AZStd::string::format("Material property name \"%s\" is not found in the material properties layout and will not be used.", name.GetCStr()).c_str());
            }
        }
        */

        m_propertyValues.swap(finalizedPropertyValues);

        m_isFinalized = true;
    }

    const AZStd::vector<PhysXMaterialPropertyValue>& PhysXMaterialAsset::GetPropertyValues()
    {
        // This can't be done in MaterialAssetHandler::LoadAssetData because the MaterialTypeAsset isn't necessarily loaded at that point.
        // And it can't be done in PostLoadInit() because that happens on the next frame which might be too late.
        // And overriding AssetHandler::InitAsset in MaterialAssetHandler didn't work, because there seems to be non-determinism on the order
        // of InitAsset calls when a ModelAsset references a MaterialAsset, the model gets initialized first and then fails to use the material.
        // So we finalize just-in-time when properties are accessed.
        // If we could solve the problem with InitAsset, that would be the ideal place to call Finalize() and we could make GetPropertyValues() const again.
        Finalize();

        //AZ_Assert(GetMaterialPropertiesLayout() && m_propertyValues.size() == GetMaterialPropertiesLayout()->GetPropertyCount(), "MaterialAsset should be finalized but does not have the right number of property values.");
        
        return m_propertyValues;
    }
        
    const AZStd::vector<AZStd::pair<AZ::Name, PhysXMaterialPropertyValue>>& PhysXMaterialAsset::GetRawPropertyValues() const
    {
        return m_rawPropertyValues;
    }

    void PhysXMaterialAsset::SetReady()
    {
        m_status = AssetStatus::Ready;

        // If this was created dynamically using MaterialAssetCreator (which is what calls SetReady()),
        // we need to connect to the AssetBus for reloads.
        PostLoadInit();
    }

    bool PhysXMaterialAsset::PostLoadInit()
    {
        if (!m_materialTypeAsset.Get())
        {
            AZ::RPI::AssetInitBus::Handler::BusDisconnect();

            // Any MaterialAsset with invalid MaterialTypeAsset is not a successfully-loaded asset.
            return false;
        }
        else
        {
            AZ::Data::AssetBus::Handler::BusConnect(m_materialTypeAsset.GetId());
            //PhysXMaterialReloadNotificationBus::Handler::BusConnect(m_materialTypeAsset.GetId());
                
            AZ::RPI::AssetInitBus::Handler::BusDisconnect();

            return true;
        }
    }

    /*void PhysXMaterialAsset::OnMaterialTypeAssetReinitialized(const AZ::Data::Asset<PhysXMaterialTypeAsset>& materialTypeAsset)
    {
        // When reloads occur, it's possible for old Asset objects to hang around and report reinitialization,
        // so we can reduce unnecessary reinitialization in that case.
        if (materialTypeAsset.Get() == m_materialTypeAsset.Get())
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnMaterialTypeAssetReinitialized %s", this, materialTypeAsset.GetHint().c_str());

            // MaterialAsset doesn't need to reinitialize any of its own data when MaterialTypeAsset reinitializes,
            // because all it depends on is the MaterialTypeAsset reference, rather than the data inside it.
            // Ultimately it's the Material that cares about these changes, so we just forward any signal we get.
            MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
        }
    }*/

    void PhysXMaterialAsset::ReinitializeMaterialTypeAsset(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::Asset<PhysXMaterialTypeAsset> newMaterialTypeAsset = AZ::Data::static_pointer_cast<PhysXMaterialTypeAsset>(asset);

        if (newMaterialTypeAsset)
        {
            // The order of asset reloads is non-deterministic. If the MaterialAsset reloads before the
            // MaterialTypeAsset, this will make sure the MaterialAsset gets update with latest one.
            // This also covers the case where just the MaterialTypeAsset is reloaded and not the MaterialAsset.
            m_materialTypeAsset = newMaterialTypeAsset;

            // If the material asset was not finalized on disk, then we clear the previously finalized property values to force re-finalize.
            // This is necessary in case the property layout changed in some way.
            if (!m_wasPreFinalized)
            {
                m_isFinalized = false;
                m_propertyValues.clear();
            }

            // Notify interested parties that this MaterialAsset is changed and may require other data to reinitialize as well
            //MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
        }
    }

    void PhysXMaterialAsset::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        //ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnAssetReloaded %s", this, asset.GetHint().c_str());
        ReinitializeMaterialTypeAsset(asset);
    }

    void PhysXMaterialAsset::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Regarding why we listen to both OnAssetReloaded and OnAssetReady, see explanation in ShaderAsset::OnAssetReady.
        //ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnAssetReady %s", this, asset.GetHint().c_str());
        ReinitializeMaterialTypeAsset(asset);
    }

    AZ::Data::AssetHandler::LoadResult PhysXMaterialAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == AZ::Data::AssetHandler::LoadResult::LoadComplete)
        {
            asset.GetAs<PhysXMaterialAsset>()->AZ::RPI::AssetInitBus::Handler::BusConnect();
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        return AZ::Data::AssetHandler::LoadResult::Error;
    }
} // namespace PhysX
