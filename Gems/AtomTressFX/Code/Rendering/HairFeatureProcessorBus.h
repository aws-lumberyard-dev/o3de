/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        //! Components that own and update a HairRenderObject should inherit from the
        //!   HairFeatureProcessorNotificationBus to receive the bone transforms update event
        
        [To Do] Adi: Make sure this is connected to retrieve the bone matrices for the Hair. 
        class HairFeatureProcessorNotifications
            : public AZ::EBusTraits
        {
        public :
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            virtual void OnUpdateSkinningMatrices() = 0;
        };
        using HairFeatureProcessorNotificationBus = AZ::EBus<HairFeatureProcessorNotifications>;
    } // namespace Render
} // namespace AZ
