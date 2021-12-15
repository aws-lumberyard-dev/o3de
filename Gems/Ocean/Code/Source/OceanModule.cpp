// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}


#include <OceanModuleInterface.h>
#include <OceanSystemComponent.h>

namespace Ocean
{
    class OceanModule
        : public OceanModuleInterface
    {
    public:
        AZ_RTTI(OceanModule, "{2e2f6a6c-edbb-4de2-8023-35b756ed77fe}", OceanModuleInterface);
        AZ_CLASS_ALLOCATOR(OceanModule, AZ::SystemAllocator, 0);
    };
}// namespace Ocean

AZ_DECLARE_MODULE_CLASS(Gem_Ocean, Ocean::OceanModule)
