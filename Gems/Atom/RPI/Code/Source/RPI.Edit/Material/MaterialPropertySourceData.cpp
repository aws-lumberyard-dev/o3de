/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertySourceData.h>

#include <Atom/RPI.Edit/Material/MaterialPropertySerializer.h>
//#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyConnectionSerializer.h>
//#include <Atom/RPI.Edit/Material/MaterialPropertyGroupSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
//#include <Atom/RPI.Edit/Material/MaterialUtils.h>
//
//#include <Atom/RPI.Edit/Common/AssetUtils.h>
//#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
//#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
//#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
//#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
//#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
//#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
//#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
//
//#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialPropertySourceData::Reflect(ReflectContext* context)
        {
            if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonMaterialPropertySerializer>()->HandlesType<MaterialPropertySourceData>();
                jsonContext->Serializer<JsonMaterialPropertyConnectionSerializer>()->HandlesType<MaterialPropertySourceData::Connection>();
                jsonContext->Serializer<JsonMaterialPropertyValueSerializer>()->HandlesType<MaterialPropertyValue>();
            }
            else if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Connection>()->Version(3);
                serializeContext->Class<MaterialPropertySourceData>()->Version(1);

                serializeContext->RegisterGenericType<AZStd::unique_ptr<MaterialPropertySourceData>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZStd::unique_ptr<MaterialPropertySourceData>>>();
                serializeContext->RegisterGenericType<ConnectionList>();
            }
        }

        MaterialPropertySourceData::Connection::Connection(MaterialPropertyOutputType type, AZStd::string_view name)
            : m_type(type)
            , m_name(name)
        {
        }

        bool MaterialPropertySourceData::Connection::operator==(const Connection& rhs) const
        {
            return m_type == rhs.m_type && m_name == rhs.m_name;
        }

        bool MaterialPropertySourceData::Connection::operator!=(const Connection& rhs) const
        {
            return!(*this == rhs);
        }

        const float MaterialPropertySourceData::DefaultMin = std::numeric_limits<float>::lowest();
        const float MaterialPropertySourceData::DefaultMax = std::numeric_limits<float>::max();
        const float MaterialPropertySourceData::DefaultStep = 0.1f;

        bool MaterialPropertySourceData::operator==(const MaterialPropertySourceData& rhs) const
        {
            return
                m_name == rhs.m_name &&
                m_visibility == rhs.m_visibility &&
                m_dataType == rhs.m_dataType &&
                m_outputConnections == rhs.m_outputConnections &&
                m_value == rhs.m_value &&
                m_enumValues == rhs.m_enumValues &&
                m_enumIsUv == rhs.m_enumIsUv &&
                m_displayName == rhs.m_displayName &&
                m_description == rhs.m_description &&
                m_vectorLabels == rhs.m_vectorLabels &&
                m_min == rhs.m_min &&
                m_max == rhs.m_max &&
                m_softMin == rhs.m_softMin &&
                m_softMax == rhs.m_softMax &&
                m_step == rhs.m_step;
        }

        bool MaterialPropertySourceData::operator!=(const MaterialPropertySourceData& rhs) const
        {
            return !(*this == rhs);
        }

 //       MaterialPropertySourceData* MaterialPropertySourceData::AddPropertyToList(AZStd::string_view name, MaterialPropertySourceDataList& list)
 //       {
 //           if (!MaterialUtils::CheckIsValidPropertyName(name))
 //           {
 //               return nullptr;
 //           }
 //
 //           auto propertyIter = AZStd::find_if(list.begin(), list.end(), [name](const AZStd::unique_ptr<MaterialPropertySourceData>& existingProperty)
 //               {
 //                   return existingProperty->GetName() == name;
 //               });
 //
 //           if (propertyIter != list.end())
 //           {
 //               AZ_Error(MaterialTypeSourceDataDebugName, false, "PropertyGroup '%s' already contains a property named '%.*s'", m_name.c_str(), AZ_STRING_ARG(name));
 //               return nullptr;
 //           }
 //
 //           auto propertyGroupIter = AZStd::find_if(m_propertyGroups.begin(), m_propertyGroups.end(), [name](const AZStd::unique_ptr<PropertyGroup>& existingPropertyGroup)
 //               {
 //                   return existingPropertyGroup->m_name == name;
 //               });
 //
 //           if (propertyGroupIter != m_propertyGroups.end())
 //           {
 //               AZ_Error(MaterialTypeSourceDataDebugName, false, "Property name '%.*s' collides with a PropertyGroup of the same name", AZ_STRING_ARG(name));
 //               return nullptr;
 //           }
 //
 //           list.emplace_back(AZStd::make_unique<MaterialPropertySourceData>(name));
 //           return list.back().get();
 //       }

    } // namespace RPI
} // namespace AZ
