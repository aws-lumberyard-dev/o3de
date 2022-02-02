/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/span.h>
//#include <Atom/RPI.Reflect/Base.h>
//#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
//#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
//#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>

namespace PhysX
{
    class PhysXMaterialTypeAsset;
    class PhysXMaterialTypeAssetCreator;
    class JsonMaterialPropertySerializer;

    //! This is a simple data structure for serializing in/out material type source files.
    //! Note that there may be a mixture of public and private members, as we are gradually introducing a proper API.
    class PhysXMaterialTypeSourceData final
    {
    public:
        AZ_TYPE_INFO(PhysX::PhysXMaterialTypeSourceData, "{BD0E1448-8918-455E-B4F6-E84A35925E8F}");
        AZ_CLASS_ALLOCATOR(PhysXMaterialTypeSourceData, AZ::SystemAllocator, 0);

        static constexpr const char Extension[] = "physxmaterialtype";

        static void Reflect(AZ::ReflectContext* context);

        struct GroupDefinition
        {
            AZ_TYPE_INFO(PhysX::MaterialTypeSourceData::GroupDefinition, "{19C47E43-619B-4B10-9266-246EC82E08F2}");

            //! The unique name of the property group. The full property ID will be groupName.propertyName
            AZStd::string m_name;

            // Editor metadata ...
            AZStd::string m_displayName;
            AZStd::string m_description;
        };

        struct PropertyDefinition
        {
            friend class JsonPhysXMaterialPropertySerializer;

            AZ_CLASS_ALLOCATOR(PropertyDefinition, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(PhysX::PhysXMaterialTypeSourceData::PropertyDefinition, "{F57816CB-C3B7-48CC-9EC0-65A8A18F6DFF}");
                
            static const float DefaultMin;
            static const float DefaultMax;
            static const float DefaultStep;
                
            PropertyDefinition() = default;

            explicit PropertyDefinition(AZStd::string_view name) : m_name(name)
            {
            }

            const AZStd::string& GetName() const { return m_name; }

            PhysXMaterialPropertyDataType m_dataType = PhysXMaterialPropertyDataType::Invalid;

            PhysXMaterialPropertyValue m_value; //!< Value for the property. The type must match the MaterialPropertyDataType.

            AZStd::vector<AZStd::string> m_enumValues; //!< Only used if property is Enum type

            // Editor metadata ...
            AZStd::string m_displayName;
            AZStd::string m_description;
            AZStd::vector<AZStd::string> m_vectorLabels;
            PhysXMaterialPropertyValue m_min;
            PhysXMaterialPropertyValue m_max;
            PhysXMaterialPropertyValue m_softMin;
            PhysXMaterialPropertyValue m_softMax;
            PhysXMaterialPropertyValue m_step;

        private:

            // We are gradually moving toward having a more proper API for MaterialTypeSourceData code, but we still some public members
            // like above. However, it's important for m_name to be private because it is used as the key for lookups, collision validation, etc.
            AZStd::string m_name; //!< The name of the property within the property group. The full property ID will be groupName.propertyName.
        };
            
        using PropertyList = AZStd::vector<AZStd::unique_ptr<PropertyDefinition>>;

        struct PropertyGroup
        {
            friend class PhysXMaterialTypeSourceData;
                
            AZ_CLASS_ALLOCATOR(PropertyGroup, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(PhysX::PhysXMaterialTypeSourceData::PropertyGroup, "{8CB6180C-5C99-4BF3-B334-A51D383314AB}");

        public:

            PropertyGroup() = default;
            AZ_DISABLE_COPY(PropertyGroup)

            const AZStd::string& GetName() const { return m_name; }
            const AZStd::string& GetDisplayName() const { return m_displayName; }
            const AZStd::string& GetDescription() const { return m_description; }
            const PropertyList& GetProperties() const { return m_properties; }
            const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& GetPropertyGroups() const { return m_propertyGroups; }
                
            void SetDisplayName(AZStd::string_view displayName) { m_displayName = displayName; }
            void SetDescription(AZStd::string_view description) { m_description = description; }

            //! Add a new property to this PropertyGroup.
            //! @param name a unique for the property. Must be a C-style identifier.
            //! @return the new PropertyDefinition, or null if the name was not valid.
            PropertyDefinition* AddProperty(AZStd::string_view name);
                
            //! Add a new nested PropertyGroup to this PropertyGroup.
            //! @param name a unique for the property group. Must be a C-style identifier.
            //! @return the new PropertyGroup, or null if the name was not valid.
            PropertyGroup* AddPropertyGroup(AZStd::string_view name);
                
        private:

            static PropertyGroup* AddPropertyGroup(AZStd::string_view name, AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& toPropertyGroupList);

            AZStd::string m_name;
            AZStd::string m_displayName;
            AZStd::string m_description;
            PropertyList m_properties;
            AZStd::vector<AZStd::unique_ptr<PropertyGroup>> m_propertyGroups;
        };

        struct PropertyLayout
        {
            AZ_TYPE_INFO(PhysX::PhysXMaterialTypeSourceData::PropertyLayout, "{1F5CDA41-E0DE-4322-8D23-9DDC445F303F}");

            PropertyLayout() = default;
            AZ_DISABLE_COPY(PropertyLayout)

            //! This field is unused, and has been replaced by MaterialTypeSourceData::m_version below. It is kept for legacy file compatibility to suppress warnings and errors.
            uint32_t m_versionOld = 0;

            //! [Deprecated] Use m_propertyGroups instead
            //! List of groups that will contain the available properties
            AZStd::vector<GroupDefinition> m_groupsOld;

            //! [Deprecated] Use m_propertyGroups instead
            AZStd::map<AZStd::string /*group name*/, AZStd::vector<PropertyDefinition>> m_propertiesOld;
                
            //! Collection of all available user-facing properties
            AZStd::vector<AZStd::unique_ptr<PropertyGroup>> m_propertyGroups;
        };
            
        AZStd::string m_description;

        //! Version 1 is the default and should not contain any version update.
        uint32_t m_version = 1;

        //! Add a new PropertyGroup for containing properties or other PropertyGroups.
        //! @param propertyGroupId The ID of the new property group. To add as a nested PropertyGroup, use a full path ID like "levelA.levelB.levelC"; in this case a property group "levelA.levelB" must already exist.
        //! @return a pointer to the new PropertyGroup or null if there was a problem (an AZ_Error will be reported).
        PropertyGroup* AddPropertyGroup(AZStd::string_view propertyGroupId);

        //! Add a new property to a PropertyGroup.
        //! @param propertyId The ID of the new property, like "layerBlend.factor" or "layer2.roughness.texture". The indicated property group must already exist.
        //! @return a pointer to the new PropertyDefinition or null if there was a problem (an AZ_Error will be reported).
        PropertyDefinition* AddProperty(AZStd::string_view propertyId);

        //! Return the PropertyLayout containing the tree of property groups and property definitions.
        const PropertyLayout& GetPropertyLayout() const { return m_propertyLayout; }

        //! Find the PropertyGroup with the given ID.
        //! @param propertyGroupId The full ID of a property group to find, like "levelA.levelB.levelC".
        //! @return the found PropertyGroup or null if it doesn't exist.
        const PropertyGroup* FindPropertyGroup(AZStd::string_view propertyGroupId) const;
        PropertyGroup* FindPropertyGroup(AZStd::string_view propertyGroupId);
            
        //! Find the definition for a property with the given ID.
        //! @param propertyId The full ID of a property to find, like "baseColor.texture".
        //! @return the found PropertyDefinition or null if it doesn't exist.
        const PropertyDefinition* FindProperty(AZStd::string_view propertyId) const;
        PropertyDefinition* FindProperty(AZStd::string_view propertyId);

        //! Tokenizes an ID string like "itemA.itemB.itemC" into a vector like ["itemA", "itemB", "itemC"].
        static AZStd::vector<AZStd::string_view> TokenizeId(AZStd::string_view id);
            
        //! Splits an ID string like "itemA.itemB.itemC" into a vector like ["itemA.itemB", "itemC"].
        static AZStd::vector<AZStd::string_view> SplitId(AZStd::string_view id);

        //! Call back function type used with the enumeration functions.
        //! Return false to terminate the traversal.
        using EnumeratePropertyGroupsCallback = AZStd::function<bool(
            const AZStd::string&, // The property ID context (i.e. "levelA.levelB.")
            const PropertyGroup* // the next property group in the tree
            )>;

        //! Recursively traverses all of the property groups contained in the material type, executing a callback function for each.
        //! @return false if the enumeration was terminated early by the callback returning false.
        bool EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback) const;

        //! Call back function type used with the numeration functions.
        //! Return false to terminate the traversal.
        using EnumeratePropertiesCallback = AZStd::function<bool(
            const AZStd::string&, // The property ID context (i.e. "levelA.levelB."
            const PropertyDefinition* // the property definition object 
            )>;
            
        //! Recursively traverses all of the properties contained in the material type, executing a callback function for each.
        //! @return false if the enumeration was terminated early by the callback returning false.
        bool EnumerateProperties(const EnumeratePropertiesCallback& callback) const;

        AZ::Outcome<AZ::Data::Asset<PhysXMaterialTypeAsset>> CreateMaterialTypeAsset(AZ::Data::AssetId assetId, AZStd::string_view materialTypeSourceFilePath = "", bool elevateWarnings = true) const;

        //! If the data was loaded from an old format file (i.e. where "groups" and "properties" were separate sections),
        //! this converts to the new format where properties are listed inside property groups.
        bool ConvertToNewDataFormat();

    private:
                
        const PropertyGroup* FindPropertyGroup(AZStd::span<const AZStd::string_view> parsedPropertyGroupId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const;
        PropertyGroup* FindPropertyGroup(AZStd::span<AZStd::string_view> parsedPropertyGroupId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList);
            
        const PropertyDefinition* FindProperty(AZStd::span<const AZStd::string_view> parsedPropertyId, AZStd::span<const AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList) const;
        PropertyDefinition* FindProperty(AZStd::span<AZStd::string_view> parsedPropertyId, AZStd::span<AZStd::unique_ptr<PropertyGroup>> inPropertyGroupList);
            
        // Function overloads for recursion, returns false to indicate that recursion should end.
        bool EnumeratePropertyGroups(const EnumeratePropertyGroupsCallback& callback, AZStd::string propertyIdContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const;
        bool EnumerateProperties(const EnumeratePropertiesCallback& callback, AZStd::string propertyIdContext, const AZStd::vector<AZStd::unique_ptr<PropertyGroup>>& inPropertyGroupList) const;

        //! Recursively populates a material asset with properties from the tree of material property groups.
        //! @param materialTypeSourceFilePath path to the material type file that is being processed, used to look up relative paths
        //! @param propertyNameContext the accumulated prefix that should be applied to any property names encountered in the current @propertyGroup
        //! @param propertyGroup the current PropertyGroup that is being processed
        //! @return false if errors are detected and processing should abort
        bool BuildPropertyList(
            const AZStd::string& materialTypeSourceFilePath,
            PhysXMaterialTypeAssetCreator& materialTypeAssetCreator,
            AZStd::vector<AZStd::string>& propertyNameContext,
            const PhysXMaterialTypeSourceData::PropertyGroup* propertyGroup) const;
                            
        //! Construct a complete list of group definitions, including implicit groups, arranged in the same order as the source data.
        //! Groups with the same name will be consolidated into a single entry.
        //! Operates on the old format PropertyLayout::m_groups, used for conversion to the new format.
        AZStd::vector<GroupDefinition> GetOldFormatGroupDefinitionsInDisplayOrder() const;
            
        PropertyLayout m_propertyLayout;
    };

} // namespace PhysX
