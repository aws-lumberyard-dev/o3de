/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/AllocatorInterface.h>

namespace AZ
{
    /**
     * The OSAllocator is a wrapper to direct OS allocation, it simply wraps OS allocation into our
     * memory system framework
     */
    class OSAllocator
    {
    public:
        AZ_TYPE_INFO(OSAllocator, "{9F835EE3-F23C-454E-B4E3-011E2F3C8118}")

        AZ_ALLOCATOR_INTERFACE(OSAllocator)
    };

    // For backwards compatibility, now that allocators respect the std interface, there is no longer a need to wrap them
    using OSStdAllocator = OSAllocator;
}
