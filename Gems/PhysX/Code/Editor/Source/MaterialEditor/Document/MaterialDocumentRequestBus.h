/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PhysXMaterial/PhysXMaterial.h>
#include <PhysXMaterial/PhysXMaterialPropertyDescriptor.h>
#include <PhysXMaterial/MaterialAsset/PhysXMaterialAsset.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace PhysX
    {
        class MaterialSourceData;
        class MaterialTypeSourceData;
    } // namespace PhysX
} // namespace AZ

namespace PhysXMaterialEditor
{
    //! UVs are processed in a property group but will be handled differently.
    //static constexpr const char UvGroupName[] = "uvSets";

    class MaterialDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get material asset created by MaterialDocument
        virtual AZ::Data::Asset<AZ::PhysX::MaterialAsset> GetAsset() const = 0;

        //! Get material instance created from asset loaded by MaterialDocument
        virtual AZ::Data::Instance<AZ::PhysX::Material> GetInstance() const = 0;

        //! Get the internal material source data
        virtual const AZ::PhysX::MaterialSourceData* GetMaterialSourceData() const = 0;

        //! Get the internal material type source data
        virtual const AZ::PhysX::MaterialTypeSourceData* GetMaterialTypeSourceData() const = 0;

        //! Modify property value
        virtual void SetPropertyValue(const AZ::Name& propertyFullName, const AZStd::any& value) = 0;

        //! Return property value
        //! If the document is not open or the id can't be found, an invalid value is returned instead.
        virtual const AZStd::any& GetPropertyValue(const AZ::Name& propertyFullName) const = 0;
   };

    using MaterialDocumentRequestBus = AZ::EBus<MaterialDocumentRequests>;
} // namespace PhysXMaterialEditor
