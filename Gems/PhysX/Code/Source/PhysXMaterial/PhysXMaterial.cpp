/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
//#include <Atom/RPI.Public/Material/Material.h>
//#include <Atom/RPI.Public/Image/StreamingImage.h>
//#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
//#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>
//#include <Atom/RPI.Public/Shader/Shader.h>
//#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
//#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

#include <PhysXMaterial/PhysXMaterial.h>

#include <AtomCore/Instance/InstanceDatabase.h>
#include <AtomCore/Utils/ScopedValue.h>

namespace PhysX
{
    const char* PhysXMaterial::s_debugTraceName = "Material";

    AZ::Data::Instance<PhysXMaterial> PhysXMaterial::FindOrCreate(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset)
    {
        return AZ::Data::InstanceDatabase<PhysXMaterial>::Instance().FindOrCreate(
            AZ::Data::InstanceId::CreateFromAssetId(materialAsset.GetId()),
            materialAsset);
    }

    AZ::Data::Instance<PhysXMaterial> PhysXMaterial::Create(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset)
    {
        return AZ::Data::InstanceDatabase<PhysXMaterial>::Instance().FindOrCreate(
            AZ::Data::InstanceId::CreateRandom(),
            materialAsset);
    }

    AZ::Data::Instance<PhysXMaterial> PhysXMaterial::CreateInternal(PhysXMaterialAsset& materialAsset)
    {
        AZ::Data::Instance<PhysXMaterial> material = aznew PhysXMaterial();
        const ResultCode resultCode = material->Init(materialAsset);

        if (resultCode == ResultCode::Success)
        {
            return material;
        }

        return nullptr;
    }

    ResultCode PhysXMaterial::Init(PhysXMaterialAsset& materialAsset)
    {
        //AZ_PROFILE_FUNCTION(RPI);

        AZ::ScopedValue isInitializing(&m_isInitializing, true, false);

        m_materialAsset = { &materialAsset, AZ::Data::AssetLoadBehavior::PreLoad };

        /*
        m_layout = m_materialAsset->GetMaterialPropertiesLayout();
        if (!m_layout)
        {
            AZ_Error(s_debugTraceName, false, "MaterialAsset did not have a valid MaterialPropertiesLayout");
            return ResultCode::Fail;
        }
        */

        // If this Init() is actually a re-initialize, we need to re-apply any overridden property values
        // after loading the property values from the asset, so we save that data here.
        PhysXMaterialPropertyFlags prevOverrideFlags = m_propertyOverrideFlags;
        AZStd::vector<PhysXMaterialPropertyValue> prevPropertyValues = m_propertyValues;

        // The property values are cleared to their default state to ensure that SetPropertyValue() does not early-return
        // when called below. This is important when Init() is actually a re-initialize.
        m_propertyValues.clear();

        // Initialize the shader runtime data like shader constant buffers and shader variants by applying the 
        // material's property values. This will feed through the normal runtime material value-change data flow, which may
        // include custom property change handlers provided by the material type.
        //
        // This baking process could be more efficient by doing it at build-time rather than run-time. However, the 
        // architectural complexity of supporting separate asset/runtime paths for assigning buffers/images is prohibitive.
        {
            m_propertyValues.resize(materialAsset.GetPropertyValues().size());
            //AZ_Assert(m_propertyValues.size() == m_layout->GetPropertyCount(), "The number of properties in this material doesn't match the property layout");

            for (size_t i = 0; i < materialAsset.GetPropertyValues().size(); ++i)
            {
                const PhysXMaterialPropertyValue& value = materialAsset.GetPropertyValues()[i];
                PhysXMaterialPropertyIndex propertyIndex{i};
                if (!SetPropertyValue(propertyIndex, value))
                {
                    return ResultCode::Fail;
                }
            }

            AZ_Assert(materialAsset.GetPropertyValues().size() <= Material::PropertyCountMax,
                "Too many material properties. Max is %d.", Material::PropertyCountMax);
        }

        // Clear all override flags because we just loaded properties from the asset
        m_propertyOverrideFlags.reset();

        // Now apply any properties that were overridden before
        for (size_t i = 0; i < prevPropertyValues.size(); ++i)
        {
            if (prevOverrideFlags[i])
            {
                SetPropertyValue(PhysXMaterialPropertyIndex{i}, prevPropertyValues[i]);
            }
        }

        // Usually SetProperties called above will increment this change ID to invalidate
        // the material, but some materials might not have any properties, and we need
        // the material to be invalidated particularly when hot-reloading.
        ++m_currentChangeId;
        // Set all dirty for the first use.
        m_propertyDirtyFlags.set();



        Compile();

        AZ::Data::AssetBus::Handler::BusConnect(m_materialAsset.GetId());
        //MaterialReloadNotificationBus::Handler::BusConnect(m_materialAsset.GetId());

        return ResultCode::Success;
    }

    PhysXMaterial::~PhysXMaterial()
    {
        //MaterialReloadNotificationBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    const AZ::Data::Asset<PhysXMaterialAsset>& PhysXMaterial::GetAsset() const
    {
        return m_materialAsset;
    }

    bool PhysXMaterial::CanCompile() const
    {
        //return !m_shaderResourceGroup || !m_shaderResourceGroup->IsQueuedForCompile();
        return true;
    }


    ///////////////////////////////////////////////////////////////////
    // AssetBus overrides...
    void PhysXMaterial::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::Asset<PhysXMaterialAsset> newMaterialAsset = AZ::Data::static_pointer_cast<PhysXMaterialAsset>(asset);

        if (newMaterialAsset)
        {
            Init(*newMaterialAsset);
            //MaterialReloadNotificationBus::Event(newMaterialAsset.GetId(), &MaterialReloadNotifications::OnMaterialReinitialized, this);
        }
    }

    ///////////////////////////////////////////////////////////////////
    // MaterialReloadNotificationBus overrides...
    /*
    void PhysXMaterial::OnMaterialAssetReinitialized(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset)
    {
        // It's important that we don't just pass materialAsset to Init() because when reloads occur,
        // it's possible for old Asset objects to hang around and report reinitialization, so materialAsset
        // might be stale data.

        if (materialAsset.Get() == m_materialAsset.Get())
        {
            OnAssetReloaded(m_materialAsset);
        }
    }
    */

    const PhysXMaterialPropertyValue& PhysXMaterial::GetPropertyValue(PhysXMaterialPropertyIndex index) const
    {
        static const PhysXMaterialPropertyValue emptyValue;
        if (m_propertyValues.size() <= index.GetIndex())
        {
            AZ_Error("Material", false, "Property index out of range.");
            return emptyValue;
        }
        return m_propertyValues[index.GetIndex()];
    }

    const AZStd::vector<PhysXMaterialPropertyValue>& PhysXMaterial::GetPropertyValues() const
    {
        return m_propertyValues;
    }

    bool PhysXMaterial::NeedsCompile() const
    {
        return m_compiledChangeId != m_currentChangeId;
    }

    bool PhysXMaterial::Compile()
    {
        //AZ_PROFILE_FUNCTION(RPI);

        if (NeedsCompile() && CanCompile())
        {
            m_propertyDirtyFlags.reset();

            m_compiledChangeId = m_currentChangeId;

            return true;
        }

        return false;
    }

    PhysXMaterial::ChangeId PhysXMaterial::GetCurrentChangeId() const
    {
        return m_currentChangeId;
    }

    PhysXMaterialPropertyIndex PhysXMaterial::FindPropertyIndex(
        [[maybe_unused]] const AZ::Name& propertyId, bool* wasRenamed, [[maybe_unused]] AZ::Name* newName) const
    {
        if (wasRenamed)
        {
            *wasRenamed = false;
        }

        /*PhysXMaterialPropertyIndex index = m_layout->FindPropertyIndex(propertyId);
        if (!index.IsValid())
        {
            AZ::Name renamedId = propertyId;
                
            if (m_materialAsset->GetMaterialTypeAsset()->ApplyPropertyRenames(renamedId))
            {                                
                index = m_layout->FindPropertyIndex(renamedId);

                if (wasRenamed)
                {
                    *wasRenamed = true;
                }

                if (newName)
                {
                    *newName = renamedId;
                }

                AZ_Warning("Material", false,
                    "Material property '%s' has been renamed to '%s'. Consider updating the corresponding source data.",
                    propertyId.GetCStr(),
                    renamedId.GetCStr());
            }
        }
        return index;*/
        return {};
    }

    template<typename Type>
    bool PhysXMaterial::SetPropertyValue(PhysXMaterialPropertyIndex index, const Type& value)
    {
        if (!index.IsValid())
        {
            AZ_Assert(false, "SetPropertyValue: Invalid MaterialPropertyIndex");
            return false;
        }

        /*const PhysXMaterialPropertyDescriptor* propertyDescriptor = m_layout->GetPropertyDescriptor(index);

        if (!ValidatePropertyAccess<Type>(propertyDescriptor))
        {
            return false;
        }*/

        PhysXMaterialPropertyValue& savedPropertyValue = m_propertyValues[index.GetIndex()];

        // If the property value didn't actually change, don't waste time running functors and compiling the changes.
        if (savedPropertyValue == value)
        {
            return false;
        }

        savedPropertyValue = value;
        m_propertyDirtyFlags.set(index.GetIndex());
        m_propertyOverrideFlags.set(index.GetIndex());

        // TODO: Is this function still needed? with descriptor out it seems to do nothing
        /*
        for(auto& outputId : propertyDescriptor->GetOutputConnections())
        {
            if (outputId.m_type == MaterialPropertyOutputType::ShaderInput)
            {
                if (propertyDescriptor->GetDataType() == MaterialPropertyDataType::Image)
                {
                    const Data::Instance<Image>& image = savedPropertyValue.GetValue<Data::Instance<Image>>();

                    RHI::ShaderInputImageIndex shaderInputIndex(outputId.m_itemIndex.GetIndex());
                    m_shaderResourceGroup->SetImage(shaderInputIndex, image);
                }
                else
                {
                    RHI::ShaderInputConstantIndex shaderInputIndex(outputId.m_itemIndex.GetIndex());
                    SetShaderConstant(shaderInputIndex, value);
                }
            }
            else if (outputId.m_type == MaterialPropertyOutputType::ShaderOption)
            {
                ShaderCollection::Item& shaderReference = m_shaderCollection[outputId.m_containerIndex.GetIndex()];
                if (!SetShaderOption(*shaderReference.GetShaderOptions(), ShaderOptionIndex{outputId.m_itemIndex.GetIndex()}, value))
                {
                    return false;
                }
            }
            else
            {
                AZ_Assert(false, "Unhandled MaterialPropertyOutputType");
                return false;
            }
        }
        */

        ++m_currentChangeId;

        return true;
    }

    // Using explicit instantiation to restrict SetPropertyValue to the set of types that we support

    template bool PhysXMaterial::SetPropertyValue<bool>     (PhysXMaterialPropertyIndex index, const bool&     value);
    template bool PhysXMaterial::SetPropertyValue<int32_t>  (PhysXMaterialPropertyIndex index, const int32_t&  value);
    template bool PhysXMaterial::SetPropertyValue<uint32_t> (PhysXMaterialPropertyIndex index, const uint32_t& value);
    template bool PhysXMaterial::SetPropertyValue<float>    (PhysXMaterialPropertyIndex index, const float&    value);
    template bool PhysXMaterial::SetPropertyValue<AZ::Vector2>  (PhysXMaterialPropertyIndex index, const AZ::Vector2&  value);
    template bool PhysXMaterial::SetPropertyValue<AZ::Vector3>  (PhysXMaterialPropertyIndex index, const AZ::Vector3&  value);
    template bool PhysXMaterial::SetPropertyValue<AZ::Vector4>  (PhysXMaterialPropertyIndex index, const AZ::Vector4&  value);
    template bool PhysXMaterial::SetPropertyValue<AZ::Color>    (PhysXMaterialPropertyIndex index, const AZ::Color&    value);

    bool PhysXMaterial::SetPropertyValue(PhysXMaterialPropertyIndex propertyIndex, const PhysXMaterialPropertyValue& value)
    {
        if (!value.IsValid())
        {
            /*
            auto descriptor = m_layout->GetPropertyDescriptor(propertyIndex);
            if (descriptor)
            {
                AZ_Assert(false, "Empty value found for material property '%s'", descriptor->GetName().GetCStr());
            }
            else
            {
                AZ_Assert(false, "Empty value found for material property [%d], and this property does not have a descriptor.");
            }
            */
            return false;
        }
        if (value.Is<bool>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<bool>());
        }
        else if (value.Is<int32_t>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<int32_t>());
        }
        else if (value.Is<uint32_t>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<uint32_t>());
        }
        else if (value.Is<float>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<float>());
        }
        else if (value.Is<AZ::Vector2>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<AZ::Vector2>());
        }
        else if (value.Is<AZ::Vector3>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<AZ::Vector3>());
        }
        else if (value.Is<AZ::Vector4>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<AZ::Vector4>());
        }
        else if (value.Is<AZ::Color>())
        {
            return SetPropertyValue(propertyIndex, value.GetValue<AZ::Color>());
        }
        else
        {
            AZ_Assert(false, "Unhandled material property value type");
            return false;
        }
    }

    template<typename Type>
    const Type& PhysXMaterial::GetPropertyValue(PhysXMaterialPropertyIndex index) const
    {
        static const Type defaultValue{};

        /*
        const PhysXMaterialPropertyDescriptor* propertyDescriptor = nullptr;
        if (Validation::IsEnabled())
        {
            if (!index.IsValid())
            {
                AZ_Assert(false, "GetPropertyValue: Invalid MaterialPropertyIndex");
                return defaultValue;
            }

            propertyDescriptor = m_layout->GetPropertyDescriptor(index);

            if (!ValidatePropertyAccess<Type>(propertyDescriptor))
            {
                return defaultValue;
            }
        }
        */

        const PhysXMaterialPropertyValue& value = m_propertyValues[index.GetIndex()];
        if (value.Is<Type>())
        {
            return value.GetValue<Type>();
        }
        else
        {
            /*if (Validation::IsEnabled())
            {
                AZ_Assert(false, "Material property '%s': Stored property value has the wrong data type. Expected %s but is %s.",
                propertyDescriptor->GetName().GetCStr(),
                    azrtti_typeid<Type>().template ToString<AZStd::string>().data(), // 'template' because clang says "error: use 'template' keyword to treat 'ToString' as a dependent template name"
                    value.GetTypeId().ToString<AZStd::string>().data());
            }*/
            return defaultValue;
        }
    }

    // Using explicit instantiation to restrict GetPropertyValue to the set of types that we support

    template const bool&     PhysXMaterial::GetPropertyValue<bool>     (PhysXMaterialPropertyIndex index) const;
    template const int32_t&  PhysXMaterial::GetPropertyValue<int32_t>  (PhysXMaterialPropertyIndex index) const;
    template const uint32_t& PhysXMaterial::GetPropertyValue<uint32_t> (PhysXMaterialPropertyIndex index) const;
    template const float&    PhysXMaterial::GetPropertyValue<float>    (PhysXMaterialPropertyIndex index) const;
    template const AZ::Vector2&  PhysXMaterial::GetPropertyValue<AZ::Vector2>  (PhysXMaterialPropertyIndex index) const;
    template const AZ::Vector3&  PhysXMaterial::GetPropertyValue<AZ::Vector3>  (PhysXMaterialPropertyIndex index) const;
    template const AZ::Vector4&  PhysXMaterial::GetPropertyValue<AZ::Vector4>  (PhysXMaterialPropertyIndex index) const;
    template const AZ::Color&    PhysXMaterial::GetPropertyValue<AZ::Color>    (PhysXMaterialPropertyIndex index) const;

    const PhysXMaterialPropertyFlags& PhysXMaterial::GetPropertyDirtyFlags() const
    {
        return m_propertyDirtyFlags;
    }
} // namespace PhysX
