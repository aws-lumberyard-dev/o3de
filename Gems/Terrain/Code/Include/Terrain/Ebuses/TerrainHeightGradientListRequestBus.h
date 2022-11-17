/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>

namespace Terrain
{
    //! This bus configures Terrain Height Gradients.
    //! This bus uses shared dispatches, which means that all requests on the bus can run in parallel,
    //! but will NOT run in parallel with bus connections / disconnections.
    class TerrainHeightGradientListRequests : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(TerrainHeightGradientListRequests, "{D2F7FCA5-64CC-4261-88A5-0FEA0A653350}");        

        ~TerrainHeightGradientListRequests() override = default;

        virtual const AZStd::vector<AZ::EntityId>& GetGradientEntities() const = 0;

        virtual void SetGradientEntities(const AZStd::vector<AZ::EntityId>& gradientEntities) = 0;
    };

    using TerrainHeightGradientListRequestBus = AZ::EBus<TerrainHeightGradientListRequests>;
} // namespace Terrain
