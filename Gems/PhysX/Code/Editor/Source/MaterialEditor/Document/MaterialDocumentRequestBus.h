/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PhysXMaterial/PhysXMaterial.h>
#include <PhysXMaterial/MaterialAsset/PhysXMaterialAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AtomCore/Instance/Instance.h>

namespace PhysX
{
    class PhysXMaterialSourceData;
    class PhysXMaterialTypeSourceData;

    class MaterialDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get material asset created by MaterialDocument
        virtual AZ::Data::Asset<PhysXMaterialAsset> GetAsset() const = 0;

        //! Get material instance created from asset loaded by MaterialDocument
        virtual AZ::Data::Instance<PhysXMaterial> GetInstance() const = 0;

        //! Get the internal material source data
        virtual const PhysXMaterialSourceData* GetMaterialSourceData() const = 0;

        //! Get the internal material type source data
        virtual const PhysXMaterialTypeSourceData* GetMaterialTypeSourceData() const = 0;
    };

    using MaterialDocumentRequestBus = AZ::EBus<MaterialDocumentRequests>;
} // namespace PhysX
