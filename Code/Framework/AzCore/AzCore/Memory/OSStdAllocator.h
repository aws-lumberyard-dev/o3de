/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocatorWrappers.h>

namespace AZ
{
    // For backwards compatibility, now that allocators respect the std interface, there is no longer a need to wrap them
    using OSStdAllocator = AllocatorGlobalWrapper<OSAllocator>;
}
