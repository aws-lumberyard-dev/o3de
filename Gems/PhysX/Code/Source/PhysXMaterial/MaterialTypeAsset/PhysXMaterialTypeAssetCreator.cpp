/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysXMaterial/MaterialTypeAsset/PhysXMaterialTypeAssetCreator.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Reflect/Limits.h>

namespace PhysX
{
    void PhysXMaterialTypeAssetCreator::Begin(const AZ::Data::AssetId& assetId)
    {
        BeginCommon(assetId);

        if (ValidateIsReady())
        {
            //m_materialPropertiesLayout = aznew PhysXMaterialPropertiesLayout;
            //m_asset->m_materialPropertiesLayout = m_materialPropertiesLayout;
        }
    }

    bool PhysXMaterialTypeAssetCreator::End(AZ::Data::Asset<PhysXMaterialTypeAsset>& result)
    {
        if (!ValidateIsReady() || !ValidateEndMaterialProperty() || !ValidateMaterialVersion())
        {
            return false; 
        }

        m_asset->SetReady();

        //m_materialShaderResourceGroupLayout = nullptr;
        //m_materialPropertiesLayout = nullptr;

        return EndCommon(result);
    }

    bool PhysXMaterialTypeAssetCreator::ValidateMaterialVersion()
    {
        return true;
    }

    void PhysXMaterialTypeAssetCreator::SetVersion(uint32_t version)
    {
        m_asset->m_version = version;
    }

    /*
    const PhysXMaterialPropertiesLayout* PhysXMaterialTypeAssetCreator::GetMaterialPropertiesLayout() const
    {
        return m_materialPropertiesLayout;
    }
    */

    // TODO: Is layout or descriptor needed for properties?
    /*
    void PhysXMaterialTypeAssetCreator::AddMaterialProperty(PhysXMaterialPropertyDescriptor&& materialProperty)
    {
        if (ValidateIsReady())
        {
            if (m_asset->m_propertyValues.size() >= Limits::Material::PropertyCountMax)
            {
                ReportError("Too man material properties. Max is %d.", Limits::Material::PropertyCountMax);
                return;
            }

            // Add the appropriate default property value for the property's data type.
            // Note, we use a separate switch statement from the one above just for clarity.
            switch (materialProperty.GetDataType())
            {
            case MaterialPropertyDataType::Bool:
                m_asset->m_propertyValues.emplace_back(false);
                break;
            case MaterialPropertyDataType::Int:
                m_asset->m_propertyValues.emplace_back(0);
                break;
            case MaterialPropertyDataType::UInt:
                m_asset->m_propertyValues.emplace_back(0u);
                break;
            case MaterialPropertyDataType::Float:
                m_asset->m_propertyValues.emplace_back(0.0f);
                break;
            case MaterialPropertyDataType::Vector2:
                m_asset->m_propertyValues.emplace_back(Vector2{ 0.0f, 0.0f });
                break;
            case MaterialPropertyDataType::Vector3:
                m_asset->m_propertyValues.emplace_back(Vector3{ 0.0f, 0.0f, 0.0f });
                break;
            case MaterialPropertyDataType::Vector4:
                m_asset->m_propertyValues.emplace_back(Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
                break;
            case MaterialPropertyDataType::Color:
                m_asset->m_propertyValues.emplace_back(Color{ 1.0f, 1.0f, 1.0f, 1.0f });
                break;
            case MaterialPropertyDataType::Image:
                m_asset->m_propertyValues.emplace_back(Data::Asset<ImageAsset>({}));
                break;
            case MaterialPropertyDataType::Enum:
                m_asset->m_propertyValues.emplace_back(0u);
                break;
            default:
                ReportError("Material property '%s': Data type is invalid.", materialProperty.GetName().GetCStr());
                return;
            }

            // Add the new descriptor
            MaterialPropertyIndex newIndex(static_cast<uint32_t>(m_materialPropertiesLayout->GetPropertyCount()));
            m_materialPropertiesLayout->m_materialPropertyIndexes.Insert(materialProperty.GetName(), newIndex);
            m_materialPropertiesLayout->m_materialPropertyDescriptors.emplace_back(AZStd::move(materialProperty));
        }
    }
    */

    bool PhysXMaterialTypeAssetCreator::ValidateBeginMaterialProperty()
    {
        if (!ValidateIsReady())
        {
            return false;
        }

        /*if (m_wipMaterialProperty.GetDataType() == PhysXMaterialPropertyDataType::Invalid)
        {
            ReportError("BeginMaterialProperty() must be called first.");
            return false;
        }*/

        return true;
    }

    bool PhysXMaterialTypeAssetCreator::ValidateEndMaterialProperty()
    {
        /*if (m_wipMaterialProperty.GetDataType() != PhysXMaterialPropertyDataType::Invalid)
        {
            ReportError("EndMaterialProperty() must be called first.");
            return false;
        }*/

        return true;
    }

    void PhysXMaterialTypeAssetCreator::BeginMaterialProperty(const AZ::Name& materialPropertyName, PhysXMaterialPropertyDataType dataType)
    {
        if (!ValidateIsReady())
        {
            return;
        }

        if (!ValidateEndMaterialProperty())
        {
            return;
        }

        /*if (m_materialPropertiesLayout->FindPropertyIndex(materialPropertyName).IsValid())
        {
            ReportError("Material property '%s': A property with this ID already exists.", materialPropertyName.GetCStr());
            return;
        }*/

        if (dataType == PhysXMaterialPropertyDataType::Invalid)
        {
            ReportError("Material property '%s': Data type is invalid.", materialPropertyName.GetCStr());
            return;
        }

        //m_wipMaterialProperty.m_nameId = materialPropertyName;
        //m_wipMaterialProperty.m_dataType = dataType;
    }

    void PhysXMaterialTypeAssetCreator::SetMaterialPropertyEnumNames([[maybe_unused]] const AZStd::vector<AZStd::string>& enumNames)
    {
        if (!ValidateBeginMaterialProperty())
        {
            return;
        }

        /*if (m_wipMaterialProperty.GetDataType() != PhysXMaterialPropertyDataType::Enum)
        {
            ReportError("Material property '%s' is not an enum but tries to store enum names.", m_wipMaterialProperty.GetName().GetCStr());
            return;
        }

        AZ_Assert(m_wipMaterialProperty.m_enumNames.empty(), "enumNames should be empty before storing!");
        m_wipMaterialProperty.m_enumNames.reserve(enumNames.size());
        for (const AZStd::string& enumName : enumNames)
        {
            m_wipMaterialProperty.m_enumNames.push_back(AZ::Name(enumName));
        }*/
    }

    void PhysXMaterialTypeAssetCreator::EndMaterialProperty()
    {
        if (!ValidateBeginMaterialProperty())
        {
            return;
        }

        //AddMaterialProperty(AZStd::move(m_wipMaterialProperty));

        //m_wipMaterialProperty = PhysXMaterialPropertyDescriptor{};
    }
        
    bool PhysXMaterialTypeAssetCreator::PropertyCheck(
        [[maybe_unused]] AZ::TypeId typeId, [[maybe_unused]] const AZ::Name& name)
    {
        /*PhysXMaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(name);
        if (!propertyIndex.IsValid())
        {
            ReportWarning("Material property '%s' not found", name.GetCStr());
            return false;
        }

        const PhysXMaterialPropertyDescriptor* materialPropertyDescriptor = m_materialPropertiesLayout->GetPropertyDescriptor(propertyIndex);
        if (!materialPropertyDescriptor)
        {
            ReportError("A material property index was found but the property descriptor was null");
            return false;
        }

        if (!ValidateMaterialPropertyDataType(typeId, name, materialPropertyDescriptor, [this](const char* message){ReportError("%s", message);}))
        {
            return false;
        }*/

        return true;
    }

    void PhysXMaterialTypeAssetCreator::SetPropertyValue(const AZ::Name& name, const PhysXMaterialPropertyValue& value)
    {
        if (PropertyCheck(value.GetTypeId(), name))
        {
            //PhysXMaterialPropertyIndex propertyIndex = m_materialPropertiesLayout->FindPropertyIndex(name);
            //m_asset->m_propertyValues[propertyIndex.GetIndex()] = value;
        }
    }
} // namespace PhysX
