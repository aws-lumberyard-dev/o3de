/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullScreenTrianglePass.h>

namespace Clouds
{
    class CloudsFullScreenPass 
        : public AZ::RPI::FullscreenTrianglePass
    {
        AZ_RPI_PASS(CloudsFullScreenPass);

    public:
        AZ_RTTI(CloudsFullScreenPass, "{A897307C-FAC6-4F46-B546-4A5BF68F99ED}", AZ::RPI::FullscreenTrianglePass);
        AZ_CLASS_ALLOCATOR(CloudsFullScreenPass, AZ::SystemAllocator, 0);

        static AZ::RPI::Ptr<CloudsFullScreenPass> Create(const AZ::RPI::PassDescriptor& descriptor);

    protected:
        explicit CloudsFullScreenPass(const AZ::RPI::PassDescriptor& descriptor);

        void BuildInternal() override;
    };
} // namespace Clouds
