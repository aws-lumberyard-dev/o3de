/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Passes/CloudsFullScreenPass.h>

namespace Clouds
{
    CloudsFullScreenPass::CloudsFullScreenPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
    {
    }

    AZ::RPI::Ptr<CloudsFullScreenPass> CloudsFullScreenPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<CloudsFullScreenPass> pass = aznew CloudsFullScreenPass(descriptor);
        return pass;
    }

    void CloudsFullScreenPass::BuildInternal()
    {
        AZ::RPI::FullscreenTrianglePass::BuildInternal();
    }
} // namespace Clouds
