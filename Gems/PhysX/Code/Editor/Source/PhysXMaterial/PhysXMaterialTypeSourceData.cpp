/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <Editor/Source/PhysXMaterial/Serializer/PhysXMaterialPropertySerializer.h>
#include <Editor/Source/PhysXMaterial/Serializer/PhysXMaterialPropertyGroupSerializer.h>
//#include <Atom/RPI.Edit/Material/MaterialUtils.h>

//#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <PhysXMaterial/MaterialTypeAsset/PhysXMaterialTypeAssetCreator.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
//#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace PhysX
{
    void PhysXMaterialTypeSourceData::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonPhysXMaterialPropertySerializer>()->HandlesType<PhysXMaterialTypeSourceData::PropertyDefinition>();
            jsonContext->Serializer<JsonPhysXMaterialPropertyGroupSerializer>()->HandlesType<PhysXMaterialTypeSourceData::GroupDefinition>();
        }
        else if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GroupDefinition>()->Version(4);
            serializeContext->Class<PropertyDefinition>()->Version(1);
                
            serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertyGroup>>();
            serializeContext->RegisterGenericType<AZStd::unique_ptr<PropertyDefinition>>();
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertyGroup>>>();
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<PropertyDefinition>>>();

            serializeContext->Class<PropertyGroup>()
                ->Version(1)
                ->Field("name", &PropertyGroup::m_name)
                ->Field("displayName", &PropertyGroup::m_displayName)
                ->Field("description", &PropertyGroup::m_description)
                ->Field("properties", &PropertyGroup::m_properties)
                ->Field("propertyGroups", &PropertyGroup::m_propertyGroups)
                ;

            serializeContext->Class<PropertyLayout>()
                ->Version(3) // Added propertyGroups
                ->Field("version", &PropertyLayout::m_versionOld)       //< Deprecated, preserved for backward compatibility, replaced by MaterialTypeSourceData::version
                ->Field("groups", &PropertyLayout::m_groupsOld)         //< Deprecated, preserved for backward compatibility, replaced by propertyGroups
                ->Field("properties", &PropertyLayout::m_propertiesOld) //< Deprecated, preserved for backward compatibility, replaced by propertyGroups
                ->Field("propertyGroups", &PropertyLayout::m_propertyGroups)
                ;

            serializeContext->Class<PhysXMaterialTypeSourceData>()
                ->Version(4) // Material Version Update
                ->Field("description", &PhysXMaterialTypeSourceData::m_description)
                ->Field("version", &PhysXMaterialTypeSourceData::m_version)
                ->Field("propertyLayout", &PhysXMaterialTypeSourceData::m_propertyLayout)
                ;
        }
    }
        
    const float PhysXMaterialTypeSourceData::PropertyDefinition::DefaultMin = std::numeric_limits<float>::lowest();
    const float PhysXMaterialTypeSourceData::PropertyDefinition::DefaultMax = std::numeric_limits<float>::max();
    const float PhysXMaterialTypeSourceData::PropertyDefinition::DefaultStep = 0.1f;
        
    /*static*/ PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::PropertyGroup::AddPropertyGroup(
        AZStd::string_view name, AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& toPropertyGroupList)
    {
        auto iter = AZStd::find_if(toPropertyGroupList.begin(), toPropertyGroupList.end(), [name](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
            {
                return existingPropertyGroup->m_name == name;
            });

        if (iter != toPropertyGroupList.end())
        {
            AZ_Error("Material source data", false, "PropertyGroup named '%.*s' already exists", AZ_STRING_ARG(name));
            return nullptr;
        }
            
        if (!AZ::RPI::MaterialPropertyId::IsValidName(name))
        {
            AZ_Error("Material source data", false, "'%.*s' is not a valid identifier", AZ_STRING_ARG(name));
            return nullptr;
        }

        toPropertyGroupList.push_back(AZStd::make_unique<PropertyGroup>());
        toPropertyGroupList.back()->m_name = name;
        return toPropertyGroupList.back().get();
    }

    PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::PropertyGroup::AddProperty(AZStd::string_view name)
    {
        auto propertyIter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
            {
                return existingProperty->GetName() == name;    
            });

        if (propertyIter != m_properties.end())
        {
            AZ_Error("Material source data", false, "PropertyGroup '%s' already contains a property named '%.*s'", m_name.c_str(), AZ_STRING_ARG(name));
            return nullptr;
        }
            
        auto propertyGroupIter = AZStd::find_if(m_propertyGroups.begin(), m_propertyGroups.end(), [name](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
            {
                return existingPropertyGroup->m_name == name;
            });

        if (propertyGroupIter != m_propertyGroups.end())
        {
            AZ_Error("Material source data", false, "Property name '%.*s' collides with a PropertyGroup of the same name", AZ_STRING_ARG(name));
            return nullptr;
        }

        if (!AZ::RPI::MaterialPropertyId::IsValidName(name))
        {
            AZ_Error("Material source data", false, "'%.*s' is not a valid identifier", AZ_STRING_ARG(name));
            return nullptr;
        }

        m_properties.emplace_back(AZStd::make_unique<PropertyDefinition>(name));
        return m_properties.back().get();
    }

    PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::PropertyGroup::AddPropertyGroup(AZStd::string_view name)
    {
        auto iter = AZStd::find_if(m_properties.begin(), m_properties.end(), [name](const AZStd::unique_ptr<PropertyDefinition>& existingProperty)
            {
                return existingProperty->GetName() == name;
            });

        if (iter != m_properties.end())
        {
            AZ_Error("Material source data", false, "PropertyGroup name '%.*s' collides with a Property of the same name", AZ_STRING_ARG(name));
            return nullptr;
        }

        return AddPropertyGroup(name, m_propertyGroups);
    }
        
    PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::AddPropertyGroup(AZStd::string_view propertyGroupId)
    {
        AZStd::vector<AZStd::string_view> splitPropertyGroupId = SplitId(propertyGroupId);

        if (splitPropertyGroupId.size() == 1)
        {
            return PropertyGroup::AddPropertyGroup(propertyGroupId, m_propertyLayout.m_propertyGroups);
        }

        PropertyGroup* parentPropertyGroup = FindPropertyGroup(splitPropertyGroupId[0]);
            
        if (!parentPropertyGroup)
        {
            AZ_Error("Material source data", false, "PropertyGroup '%.*s' does not exists", AZ_STRING_ARG(splitPropertyGroupId[0]));
            return nullptr;
        }

        return parentPropertyGroup->AddPropertyGroup(splitPropertyGroupId[1]);
    }
        
    PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::AddProperty(AZStd::string_view propertyId)
    {
        AZStd::vector<AZStd::string_view> splitPropertyId = SplitId(propertyId);

        if (splitPropertyId.size() == 1)
        {
            AZ_Error("Material source data", false, "Property id '%.*s' is invalid. Properties must be added to a PropertyGroup (i.e. \"general.%.*s\").", AZ_STRING_ARG(propertyId), AZ_STRING_ARG(propertyId));
            return nullptr;
        }

        PropertyGroup* parentPropertyGroup = FindPropertyGroup(splitPropertyId[0]);
            
        if (!parentPropertyGroup)
        {
            AZ_Error("Material source data", false, "PropertyGroup '%.*s' does not exists", AZ_STRING_ARG(splitPropertyId[0]));
            return nullptr;
        }

        return parentPropertyGroup->AddProperty(splitPropertyId[1]);
    }
        
    const PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::FindPropertyGroup(
        AZStd::span<const AZStd::string_view> parsedPropertyGroupId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const
    {
        for (const auto& propertyGroup : inPropertyGroupList)
        {
            if (propertyGroup->m_name != parsedPropertyGroupId[0])
            {
                continue;
            }
            else if (parsedPropertyGroupId.size() == 1)
            {
                return propertyGroup.get();
            }
            else
            {
                AZStd::span<const AZStd::string_view> subPath{parsedPropertyGroupId.begin() + 1, parsedPropertyGroupId.end()};

                if (!subPath.empty())
                {
                    const PhysXMaterialTypeSourceData::PropertyGroup* propertySubset = FindPropertyGroup(subPath, propertyGroup->m_propertyGroups);
                    if (propertySubset)
                    {
                        return propertySubset;
                    }
                }
            }
        }

        return nullptr;
    }
        
    PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::FindPropertyGroup(
        AZStd::span<AZStd::string_view> parsedPropertyGroupId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList)
    {
        return const_cast<PropertyGroup*>(const_cast<const PhysXMaterialTypeSourceData*>(this)->FindPropertyGroup(parsedPropertyGroupId, inPropertyGroupList));
    }

    const PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::FindPropertyGroup(AZStd::string_view propertyGroupId) const
    {
        AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyGroupId);
        return FindPropertyGroup(tokens, m_propertyLayout.m_propertyGroups);
    }

    PhysXMaterialTypeSourceData::PropertyGroup* PhysXMaterialTypeSourceData::FindPropertyGroup(AZStd::string_view propertyGroupId)
    {
        AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyGroupId);
        return FindPropertyGroup(tokens, m_propertyLayout.m_propertyGroups);
    }
        
    const PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::FindProperty(
        AZStd::span<const AZStd::string_view> parsedPropertyId,
        AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const
    {
        for (const auto& propertyGroup : inPropertyGroupList)
        {
            if (propertyGroup->m_name == parsedPropertyId[0])
            {
                AZStd::span<const AZStd::string_view> subPath {parsedPropertyId.begin() + 1, parsedPropertyId.end()};

                if (subPath.size() == 1)
                {
                    for (AZStd::unique_ptr<PropertyDefinition>& property : propertyGroup->m_properties)
                    {
                        if (property->GetName() == subPath[0])
                        {
                            return property.get();
                        }
                    }
                }
                else if(subPath.size() > 1)
                {
                    const PhysXMaterialTypeSourceData::PropertyDefinition* property = FindProperty(subPath, propertyGroup->m_propertyGroups);
                    if (property)
                    {
                        return property;
                    }
                }
            }
        }

        return nullptr;
    }
        
    PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::FindProperty(
        AZStd::span<AZStd::string_view> parsedPropertyId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList)
    {
        return const_cast<PhysXMaterialTypeSourceData::PropertyDefinition*>(const_cast<const PhysXMaterialTypeSourceData*>(this)->FindProperty(parsedPropertyId, inPropertyGroupList));
    }

    const PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::FindProperty(AZStd::string_view propertyId) const
    {
        AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
        return FindProperty(tokens, m_propertyLayout.m_propertyGroups);
    }
        
    PhysXMaterialTypeSourceData::PropertyDefinition* PhysXMaterialTypeSourceData::FindProperty(AZStd::string_view propertyId)
    {
        AZStd::vector<AZStd::string_view> tokens = TokenizeId(propertyId);
        return FindProperty(tokens, m_propertyLayout.m_propertyGroups);
    }

    AZStd::vector<AZStd::string_view> PhysXMaterialTypeSourceData::TokenizeId(AZStd::string_view id)
    {
        AZStd::vector<AZStd::string_view> tokens;

        AzFramework::StringFunc::TokenizeVisitor(id, [&tokens](AZStd::string_view t)
            {
                tokens.push_back(t);
            },
            "./", true, true);

        return tokens;
    }
        
    AZStd::vector<AZStd::string_view> PhysXMaterialTypeSourceData::SplitId(AZStd::string_view id)
    {
        AZStd::vector<AZStd::string_view> parts;
        parts.reserve(2);
        size_t lastDelim = id.rfind('.', id.size()-1);
        if (lastDelim == AZStd::string::npos)
        {
            parts.push_back(id);
        }
        else
        {
            parts.push_back(AZStd::string_view{id.begin(), id.begin()+lastDelim});
            parts.push_back(AZStd::string_view{id.begin()+lastDelim+1, id.end()});
        }

        return parts;
    }

    bool PhysXMaterialTypeSourceData::EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback, AZStd::string propertyNameContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const
    {
        for (auto& propertyGroup : inPropertyGroupList)
        {
            if (!callback(propertyNameContext, propertyGroup.get()))
            {
                return false; // Stop processing
            }

            const AZStd::string propertyNameContext2 = propertyNameContext + propertyGroup->m_name + ".";

            if (!EnumeratePropertyGroups(callback, propertyNameContext2, propertyGroup->m_propertyGroups))
            {
                return false; // Stop processing
            }
        }

        return true;
    }

    bool PhysXMaterialTypeSourceData::EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback) const
    {
        if (!callback)
        {
            return false;
        }

        return EnumeratePropertyGroups(callback, {}, m_propertyLayout.m_propertyGroups);
    }

    bool PhysXMaterialTypeSourceData::EnumerateProperties(
        const EnumeratePropertiesCallback& callback, AZStd::string propertyNameContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const
    {

        for (auto& propertyGroup : inPropertyGroupList)
        {
            const AZStd::string propertyNameContext2 = propertyNameContext + propertyGroup->m_name + ".";

            for (auto& property : propertyGroup->m_properties)
            {
                if (!callback(propertyNameContext2, property.get()))
                {
                    return false; // Stop processing
                }
            }

            if (!EnumerateProperties(callback, propertyNameContext2, propertyGroup->m_propertyGroups))
            {
                return false; // Stop processing
            }
        }

        return true;
    }

    bool PhysXMaterialTypeSourceData::EnumerateProperties(const EnumeratePropertiesCallback& callback) const
    {
        if (!callback)
        {
            return false;
        }

        return EnumerateProperties(callback, {}, m_propertyLayout.m_propertyGroups);
    }

    bool PhysXMaterialTypeSourceData::ConvertToNewDataFormat()
    {            
        for (const auto& group : GetOldFormatGroupDefinitionsInDisplayOrder())
        {
            auto propertyListItr = m_propertyLayout.m_propertiesOld.find(group.m_name);
            if (propertyListItr != m_propertyLayout.m_propertiesOld.end())
            {
                const auto& propertyList = propertyListItr->second;
                for (auto& propertyDefinition : propertyList)
                {
                    PropertyGroup* propertyGroup = FindPropertyGroup(group.m_name);

                    if (!propertyGroup)
                    {
                        m_propertyLayout.m_propertyGroups.emplace_back(AZStd::make_unique<PropertyGroup>());
                        m_propertyLayout.m_propertyGroups.back()->m_name = group.m_name;
                        m_propertyLayout.m_propertyGroups.back()->m_displayName = group.m_displayName;
                        m_propertyLayout.m_propertyGroups.back()->m_description = group.m_description;
                        propertyGroup = m_propertyLayout.m_propertyGroups.back().get();
                    }

                    PropertyDefinition* newProperty = propertyGroup->AddProperty(propertyDefinition.GetName());
                        
                    *newProperty = propertyDefinition; 
                }
            }
        }

        m_propertyLayout.m_groupsOld.clear();
        m_propertyLayout.m_propertiesOld.clear();

        return true;
    }

    AZStd::vector<PhysXMaterialTypeSourceData::GroupDefinition> PhysXMaterialTypeSourceData::GetOldFormatGroupDefinitionsInDisplayOrder() const
    {
        AZStd::vector<PhysXMaterialTypeSourceData::GroupDefinition> groupDefinitions;
        groupDefinitions.reserve(m_propertyLayout.m_propertiesOld.size());

        // Some groups are defined explicitly in the .materialtype file's "groups" section. This is the primary way groups are sorted in the UI.
        AZStd::unordered_set<AZStd::string> foundGroups;
        for (const auto& groupDefinition : m_propertyLayout.m_groupsOld)
        {
            if (foundGroups.insert(groupDefinition.m_name).second)
            {
                groupDefinitions.push_back(groupDefinition);
            }
            else
            {
                AZ_Warning("Material source data", false, "Duplicate group '%s' found.", groupDefinition.m_name.c_str());
            }
        }

        // Some groups are defined implicitly, in the "properties" section where a group name is used but not explicitly defined in the "groups" section.
        for (const auto& propertyListPair : m_propertyLayout.m_propertiesOld)
        {
            const AZStd::string& groupName = propertyListPair.first;
            if (foundGroups.insert(groupName).second)
            {
                PhysXMaterialTypeSourceData::GroupDefinition groupDefinition;
                groupDefinition.m_name = groupName;
                groupDefinitions.push_back(groupDefinition);
            }
        }

        return groupDefinitions;
    }

    bool PhysXMaterialTypeSourceData::BuildPropertyList(
        const AZStd::string& materialTypeSourceFilePath,
        PhysXMaterialTypeAssetCreator& materialTypeAssetCreator,
        AZStd::vector<AZStd::string>& propertyNameContext,
        const PhysXMaterialTypeSourceData::PropertyGroup* propertyGroup) const
    {            
        for (const AZStd::unique_ptr<PropertyDefinition>& property : propertyGroup->m_properties)
        {
            // Register the property...

            AZ::RPI::MaterialPropertyId propertyId{propertyNameContext, property->GetName()};

            if (!propertyId.IsValid())
            {
                // MaterialPropertyId reports an error message
                return false;
            }

            auto propertyGroupIter = AZStd::find_if(propertyGroup->GetPropertyGroups().begin(), propertyGroup->GetPropertyGroups().end(),
                [&property](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
                {
                    return existingPropertyGroup->GetName() == property->GetName();
                });

            if (propertyGroupIter != propertyGroup->GetPropertyGroups().end())
            {
                AZ_Error("Material source data", false, "Material property '%s' collides with a PropertyGroup with the same ID.", propertyId.GetCStr());
                return false;
            }

            materialTypeAssetCreator.BeginMaterialProperty(propertyId, property->m_dataType);
                
            if (property->m_dataType == PhysXMaterialPropertyDataType::Enum)
            {
                materialTypeAssetCreator.SetMaterialPropertyEnumNames(property->m_enumValues);
            }

            materialTypeAssetCreator.EndMaterialProperty();

            // Parse and set the property's value...
            if (!property->m_value.IsValid())
            {
                AZ_Warning("Material source data", false, "Source data for material property value is invalid.");
            }
            else
            {
                switch (property->m_dataType)
                {
                case PhysXMaterialPropertyDataType::Enum:
                {
                    // TODO: Try to bring back property layout
                    /*
                    PhysXMaterialPropertyIndex propertyIndex = materialTypeAssetCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
                    const PhysXMaterialPropertyDescriptor* propertyDescriptor = materialTypeAssetCreator.GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

                    AZ::Name enumName = AZ::Name(property->m_value.GetValue<AZStd::string>());
                    uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                    if (enumValue == PhysXMaterialPropertyDescriptor::InvalidEnumValue)
                    {
                        materialTypeAssetCreator.ReportError("Enum value '%s' couldn't be found in the 'enumValues' list", enumName.GetCStr());
                    }
                    else
                    {
                        materialTypeAssetCreator.SetPropertyValue(propertyId, enumValue);
                    }
                    */
                }
                break;
                default:
                    materialTypeAssetCreator.SetPropertyValue(propertyId, property->m_value);
                    break;
                }
            }
        }
            
        for (const AZStd::unique_ptr<PropertyGroup>& propertySubset : propertyGroup->m_propertyGroups)
        {
            propertyNameContext.push_back(propertySubset->m_name);

            bool success = BuildPropertyList(
                materialTypeSourceFilePath,
                materialTypeAssetCreator,
                propertyNameContext,
                propertySubset.get());

            propertyNameContext.pop_back();

            if (!success)
            {
                return false;
            }
        }

        return true;
    }


    AZ::Outcome<AZ::Data::Asset<PhysXMaterialTypeAsset>> PhysXMaterialTypeSourceData::CreateMaterialTypeAsset(
        AZ::Data::AssetId assetId, AZStd::string_view materialTypeSourceFilePath, bool elevateWarnings) const
    {
        PhysXMaterialTypeAssetCreator materialTypeAssetCreator;
        materialTypeAssetCreator.SetElevateWarnings(elevateWarnings);
        materialTypeAssetCreator.Begin(assetId);

        if (m_propertyLayout.m_versionOld != 0)
        {
            materialTypeAssetCreator.ReportError(
                "The field '/propertyLayout/version' is deprecated and moved to '/version'. "
                "Please edit this material type source file and move the '\"version\": %u' setting up one level.",
                m_propertyLayout.m_versionOld);
            return AZ::Failure();
        }

        // Set materialtype version and add each version update object into MaterialTypeAsset.
        materialTypeAssetCreator.SetVersion(m_version);

        for (const AZStd::unique_ptr<PropertyGroup>& propertyGroup : m_propertyLayout.m_propertyGroups)
        {
            AZStd::vector<AZStd::string> propertyNameContext;
            propertyNameContext.push_back(propertyGroup->m_name);
            bool success = BuildPropertyList(materialTypeSourceFilePath, materialTypeAssetCreator, propertyNameContext, propertyGroup.get());

            if (!success)
            {
                return AZ::Failure();
            }
        }

        AZ::Data::Asset<PhysXMaterialTypeAsset> materialTypeAsset;
        if (materialTypeAssetCreator.End(materialTypeAsset))
        {
            return AZ::Success(AZStd::move(materialTypeAsset));
        }
        else
        {
            return AZ::Failure();
        }
    }
} // namespace AZ
