/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

namespace WhiteBox
{
    //! White Box Gem allocator for all default allocations.
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(WhiteBoxAllocatorm, AZ::SystemAllocator, "{BFEB8C64-FDB7-4A19-B9B4-DDF57A434F14}")

} // namespace WhiteBox
