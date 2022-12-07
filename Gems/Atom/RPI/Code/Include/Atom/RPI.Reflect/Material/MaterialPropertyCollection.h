/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>

// These classes are not directly referenced in this header only because the Set/GetPropertyValue()
// functions are templatized. But the API is still specific to these data types so we include them here.
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialPropertyCollection
        {
        public:
            bool Init(
                RHI::ConstPtr<MaterialPropertiesLayout> layout,
                const AZStd::vector<MaterialPropertyValue>& defaultValues);

            //! Sets the value of a material property. The template data type must match the property's data type.
            //! @return true if property value was changed
            template<typename Type>
            bool SetPropertyValue(MaterialPropertyIndex index, const Type& value);

            //! Gets the value of a material property. The template data type must match the property's data type.
            template<typename Type>
            const Type& GetPropertyValue(MaterialPropertyIndex index) const;

            //! Sets the value of a material property. The @value data type must match the property's data type.
            //! @return true if property value was changed
            bool SetPropertyValue(MaterialPropertyIndex index, const MaterialPropertyValue& value);

            const MaterialPropertyValue& GetPropertyValue(MaterialPropertyIndex index) const;
            const AZStd::vector<MaterialPropertyValue>& GetPropertyValues() const;

            //! Gets flags indicating which properties have been modified.
            const MaterialPropertyFlags& GetPropertyDirtyFlags() const;

            void SetAllPropertyDirtyFlags();
            void ClearAllPropertyDirtyFlags();

            //! Gets the material properties layout.
            RHI::ConstPtr<MaterialPropertiesLayout> GetMaterialPropertiesLayout() const;

        private:
            template<typename Type>
            bool ValidatePropertyAccess(const MaterialPropertyDescriptor* propertyDescriptor) const;

            //! Provides a description of the set of available material properties, cached locally so we don't have to keep fetching it from the MaterialTypeSourceData.
            RHI::ConstPtr<MaterialPropertiesLayout> m_layout;

            //! Values for all properties in MaterialPropertiesLayout
            AZStd::vector<MaterialPropertyValue> m_propertyValues;

            //! Flags indicate which properties have been modified so that related functors will update.
            MaterialPropertyFlags m_propertyDirtyFlags;

            //! Used to track which properties have been modified at runtime so they can be preserved if the material has to reinitialiize.
            MaterialPropertyFlags m_propertyOverrideFlags;

        };

    } // namespace RPI
} // namespace AZ
