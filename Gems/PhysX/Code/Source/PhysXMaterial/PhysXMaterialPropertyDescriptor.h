/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Name/Name.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <PhysXMaterial/PhysXMaterialPropertyValue.h>

namespace AZ
{
    namespace PhysX
    {
        class MaterialPropertyDescriptor;

        struct MaterialPropertyIndexType {
            AZ_TYPE_INFO(AZ::PhysX::MaterialPropertyIndexType, "{897C62BC-4BA1-454D-8573-98BCAAF73D01}");
        };

        using MaterialPropertyIndex = RHI::Handle<uint32_t, AZ::PhysX::MaterialPropertyIndexType>;

        /*enum class MaterialPropertyOutputType
        {
            ShaderInput,  //!< Maps to a ShaderResourceGroup input
            ShaderOption, //!< Maps to a shader variant option
            Invalid,
            Count = Invalid
        };*/

        //static const uint32_t MaterialPropertyOutputTypeCount = static_cast<uint32_t>(MaterialPropertyOutputType::Count);
        //const char* ToString(MaterialPropertyOutputType materialPropertyOutputType);

        //! Represents a specific output data binding for the material property layer. 
        /*class MaterialPropertyOutputId
        {
        public:
            AZ_TYPE_INFO(MaterialPropertyOutputId, "{A3D2923D-8CB1-49CB-BF5D-4429A406BF1A}")

                static void Reflect(ReflectContext* context);

            MaterialPropertyOutputType m_type = MaterialPropertyOutputType::Invalid;

            //! For m_type==ShaderOption, this is the index of a specific ShaderAsset (see MaterialTypeSourceData's ShaderCollection). 
            //! For m_type==ShaderInput, this field is not used (because there is only one material ShaderResourceGroup in a MaterialAsset).
            RHI::Handle<uint32_t> m_containerIndex;

            //! Index to the specific setting that the material property maps to. 
            //! The MaterialPropertyDataType, MaterialPropertyOutputType, and m_containerIndex determine which list this refers to.
            RHI::Handle<uint32_t> m_itemIndex;
        };*/

        enum class MaterialPropertyDataType : uint32_t
        {
            Invalid,

            Bool,
            Int,
            UInt,
            Float,
            Vector2,
            Vector3,
            Vector4,
            Color,
            //Image,
            Enum,   //!< This type is only used in source data files, not runtime data

            Count
        };

        static const uint32_t MaterialPropertyDataTypeCount = static_cast<uint32_t>(MaterialPropertyDataType::Count);

        const char* ToString(MaterialPropertyDataType materialPropertyDataType);

        AZStd::string GetMaterialPropertyDataTypeString(AZ::TypeId typeId);

        //! Checks that the TypeId matches the type expected by materialPropertyDescriptor
        bool ValidateMaterialPropertyDataType(TypeId typeId, const Name& propertyName, const MaterialPropertyDescriptor* materialPropertyDescriptor, AZStd::function<void(const char*)> onError);

        //! A material property is any data input to a material, like a bool, float, Vector, Image, Buffer, etc.
        //! This descriptor defines a single input property, including it's name ID, and how it maps
        //! to the shader system.
        //!
        //! Each property can be directly connected to various outputs like ShaderResourceGroup fields, shader variant options, etc.
        //! In most cases there will be a single output connection, but multiple connections are possible.
        //! Alternatively, the property may not have any direct connections and would be processed by a custom material functor
        //! instead (see MaterialFunctor.h). Note that having direct output connections does not preclude the use of a functor;
        //! a property with a direct connection may also be processed by a material functor for additional indirect handling.
        class MaterialPropertyDescriptor
        {
            friend class MaterialTypeAssetCreator;
        public:
            AZ_TYPE_INFO(AZ::PhysX::MaterialPropertyDescriptor, "{571E97E5-EFDE-40AB-B3AB-903F241898A8}");

            //using OutputList = AZStd::vector<MaterialPropertyOutputId>;

            static void Reflect(ReflectContext* context);

            MaterialPropertyDescriptor() = default;

            MaterialPropertyDataType GetDataType() const;
            //! Returns the TypeId that is used to store values for this material property.
            AZ::TypeId GetStorageDataTypeId() const;

            //! Returns the value of the enum from its name. If this property is not an enum or the name is undefined, InvalidEnumValue is returned.
            static constexpr uint32_t InvalidEnumValue = std::numeric_limits<uint32_t>::max();
            uint32_t GetEnumValue(const AZ::Name& enumName) const;

            //! Returns the name of the enum from its index. An empty name is returned for an invalid id.
            const AZ::Name& GetEnumName(uint32_t enumValue) const;

            //! Returns the unique name ID of this property
            const Name& GetName() const;

            //! Returns the list of shader setting the property is directly connected to.
            //const OutputList& GetOutputConnections() const;

        private:
            MaterialPropertyDataType m_dataType = MaterialPropertyDataType::Invalid;
            AZStd::vector<AZ::Name> m_enumNames;
            Name m_nameId;
            //OutputList m_outputConnections;
        };
    } // namespace PhysX

    //AZ_TYPE_INFO_SPECIALIZE(AZ::PhysX::MaterialPropertyOutputType, "{571E97E5-EFDE-40AB-B3AB-903F241898A8}");
    AZ_TYPE_INFO_SPECIALIZE(AZ::PhysX::MaterialPropertyDataType, "{E2D3CE4E-2EEC-44EC-B4CC-A4FEF42C16E7}");
} // namespace AZ

