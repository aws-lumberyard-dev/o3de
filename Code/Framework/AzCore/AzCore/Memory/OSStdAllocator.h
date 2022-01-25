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
    // Stateless wrapper to the OSAllocator
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(OSStdAllocator, OSAllocator, "{DEF429A7-391E-4448-B196-E926BB4CA2A9}")
}
