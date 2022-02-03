/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

// These classes are not directly referenced in this header only because the Set/GetPropertyValue()
// functions are templatized. But the API is still specific to these data types so we include them here.
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

//#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
#include <PhysXMaterial/MaterialAsset/PhysXMaterialAsset.h>

#include <AtomCore/Instance/InstanceData.h>
#include <Atom/RHI.Reflect/Handle.h> // <=== move to AtomCore?

namespace PhysX
{
    class PhysXMaterialPropertiesLayout;

    namespace Material
    {
        constexpr uint32_t PropertyCountMax = 256;
    }
    using PhysXMaterialPropertyFlags = AZStd::bitset<Material::PropertyCountMax>;

    struct PhysXMaterialPropertyIndexType {
        AZ_TYPE_INFO(MaterialPropertyIndexType, "{44881C42-E006-4970-A391-35487271C7BF}");
    };

    using PhysXMaterialPropertyIndex = AZ::RHI::Handle<uint32_t, PhysXMaterialPropertyIndexType>;

    enum class ResultCode : uint32_t
    {
        // The operation succeeded.
        Success = 0,

        // The operation failed with an unknown error.
        Fail,

        // The operation failed due being out of memory.
        OutOfMemory,

        // The operation failed because the feature is unimplemented on the particular platform.
        Unimplemented,

        // The operation failed because the API object is not in a state to accept the call.
        InvalidOperation,

        // The operation failed due to invalid arguments.
        InvalidArgument,

        // The operation is not ready
        NotReady
    };

    //! Provides runtime material functionality based on a MaterialAsset. The material operates on a
    //! set of properties, which are configured primarily at build-time through the MaterialAsset. 
    //! These properties are used to configure shader system inputs at runtime.
    //! 
    //! Material property values can be accessed at runtime, using the SetPropertyValue() and GetPropertyValue().
    //! After applying all property changes, Compile() must be called to apply those changes to the shader system.
    //! 
    //! If RPI validation is enabled, the class will perform additional error checking. If a setter method fails
    //! an error is emitted and the call returns false without performing the requested operation. Likewise, if 
    //! a getter method fails, an error is emitted and an empty value is returned. If validation is disabled, the 
    //! operation is always performed.
    class PhysXMaterial
        : public AZ::Data::InstanceData
        , public AZ::Data::AssetBus::Handler
        //, public AZ::RPI::MaterialReloadNotificationBus::Handler
    {
    public:
        AZ_INSTANCE_DATA(PhysXMaterial, "{CDB38D9B-6D75-4D2C-A195-0F7B233DE881}");
        AZ_CLASS_ALLOCATOR(PhysXMaterial, AZ::SystemAllocator, 0);

        //! Material objects use a ChangeId to track when changes have been made to the material at runtime. See GetCurrentChangeId()
        using ChangeId = size_t;

        //! GetCurrentChangeId() will never return this value, so client code can use this to initialize a ChangeId that is immediately dirty
        static const ChangeId DEFAULT_CHANGE_ID = 0;

        static AZ::Data::Instance<PhysXMaterial> FindOrCreate(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset);
        static AZ::Data::Instance<PhysXMaterial> Create(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset);

        virtual ~PhysXMaterial();

        //! Finds the material property index from the material property ID
        //! @param wasRenamed optional parameter that is set to true if @propertyId is an old name and an automatic rename was applied to find the index.
        //! @param newName optional parameter that is set to the new property name, if the property was renamed.
        PhysXMaterialPropertyIndex FindPropertyIndex(const AZ::Name& propertyId, bool* wasRenamed = nullptr, AZ::Name* newName = nullptr) const;

        //! Sets the value of a material property. The template data type must match the property's data type.
        //! @return true if property value was changed
        template<typename Type>
        bool SetPropertyValue(PhysXMaterialPropertyIndex index, const Type& value);

        //! Gets the value of a material property. The template data type must match the property's data type.
        template<typename Type>
        const Type& GetPropertyValue(PhysXMaterialPropertyIndex index) const;

        //! Sets the value of a material property. The @value data type must match the property's data type.
        //! @return true if property value was changed
        bool SetPropertyValue(PhysXMaterialPropertyIndex index, const PhysXMaterialPropertyValue& value);

        const PhysXMaterialPropertyValue& GetPropertyValue(PhysXMaterialPropertyIndex index) const;
        const AZStd::vector<PhysXMaterialPropertyValue>& GetPropertyValues() const;
            
        //! Gets flags indicating which properties have been modified.
        const PhysXMaterialPropertyFlags& GetPropertyDirtyFlags() const;

        //! Gets the material properties layout.
        AZStd::intrusive_ptr<const PhysXMaterialPropertiesLayout> GetMaterialPropertiesLayout() const;

        //! Must be called after changing any material property values in order to apply those changes to the shader.
        //! Does nothing if NeedsCompile() is false or CanCompile() is false.
        //! @return whether compilation occurred
        bool Compile();
            
        //! Returns an ID that can be used to track whether the material has changed since the last time client code read it.
        //! This gets incremented every time a change is made, like by calling SetPropertyValue().
        ChangeId GetCurrentChangeId() const;

        const AZ::Data::Asset<PhysXMaterialAsset>& GetAsset() const;

        //! Returns whether the material is ready to compile pending changes. (Materials can only be compiled once per frame because SRGs can only be compiled once per frame).
        bool CanCompile() const;

        //! Returns whether the material has property changes that have not been compiled yet.
        bool NeedsCompile() const;

    private:
        PhysXMaterial() = default;

        //! Standard init path from asset data.
        static AZ::Data::Instance<PhysXMaterial> CreateInternal(PhysXMaterialAsset& materialAsset);
        ResultCode Init(PhysXMaterialAsset& materialAsset);

        ///////////////////////////////////////////////////////////////////
        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        ///////////////////////////////////////////////////////////////////
        // MaterialReloadNotificationBus overrides...
        //void OnMaterialAssetReinitialized(const AZ::Data::Asset<PhysXMaterialAsset>& materialAsset) override;

        static const char* s_debugTraceName;

        //! The corresponding material asset that provides material type data and initial property values.
        AZ::Data::Asset<PhysXMaterialAsset> m_materialAsset;

        //! Provides a description of the set of available material properties, cached locally so we don't have to keep fetching it from the MaterialTypeSourceData.
        //AZStd::intrusive_ptr<const PhysXMaterialPropertiesLayout> m_layout;

        //! Values for all properties in MaterialPropertiesLayout
        AZStd::vector<PhysXMaterialPropertyValue> m_propertyValues;

        //! Flags indicate which properties have been modified so that related functors will update.
        PhysXMaterialPropertyFlags m_propertyDirtyFlags;

        //! Used to track which properties have been modified at runtime so they can be preserved if the material has to reinitialiize.
        PhysXMaterialPropertyFlags m_propertyOverrideFlags;

        //! Tracks each change made to material properties.
        //! Initialized to DEFAULT_CHANGE_ID+1 to ensure that GetCurrentChangeId() will not return DEFAULT_CHANGE_ID (a value that client 
        //! code can use to initialize a ChangeId that is immediately dirty).
        ChangeId m_currentChangeId = DEFAULT_CHANGE_ID + 1;

        //! Records the m_currentChangeId when the material was last compiled.
        ChangeId m_compiledChangeId = DEFAULT_CHANGE_ID;

        bool m_isInitializing = false;
    };
} // namespace AZ
