/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PhysXMaterial/PhysXMaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
//#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace PhysX
    {
        //! Provides a set of MaterialPropertyDescriptors which define the topology for a material.
        class MaterialPropertiesLayout
            : public AZStd::intrusive_base
        {
            friend class MaterialTypeAssetCreator;
        public:
            AZ_TYPE_INFO(AZ::PhysX::MaterialPropertiesLayout, "{E6A0CB3A-8340-48E0-A13F-F2AD7CB419E1}");
            AZ_CLASS_ALLOCATOR(MaterialPropertiesLayout, SystemAllocator, 0);

            using PropertyList = AZStd::vector<MaterialPropertyDescriptor>;

            static void Reflect(ReflectContext* context);

            MaterialPropertiesLayout() = default;
            AZ_DISABLE_COPY_MOVE(MaterialPropertiesLayout);

            size_t GetPropertyCount() const;
            MaterialPropertyIndex FindPropertyIndex(const Name& propertyId) const;
            const MaterialPropertyDescriptor* GetPropertyDescriptor(MaterialPropertyIndex index) const;

        private:
            using IdReflectionMapForMaterialProperties = RHI::NameIdReflectionMap<MaterialPropertyIndex>;
            IdReflectionMapForMaterialProperties m_materialPropertyIndexes;
            PropertyList m_materialPropertyDescriptors;
        };

    } // namespace PhysX
} // namespace AZ
