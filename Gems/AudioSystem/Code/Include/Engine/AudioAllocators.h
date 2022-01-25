/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/SystemAllocator.h>
#define AUDIO_MEMORY_ALIGNMENT  AZCORE_GLOBAL_NEW_ALIGNMENT

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(AudioSystemAllocator, AZ::SystemAllocator, "{AE15F55D-BD65-4666-B18B-9ED81999A85B}")
    using AudioSystemStdAllocator = AudioSystemAllocator;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(AudioImplAllocator, AZ::SystemAllocator, "{197D999F-3093-4F9D-A9A0-BA9E2AAA11DC}")
    using AudioImplStdAllocator = AudioImplAllocator;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(AudioBankAllocator, AZ::SystemAllocatorm, "{19E89718-400F-42F9-92C3-E7F0DC1CCC1F}")

} // namespace Audio


#define AUDIO_SYSTEM_CLASS_ALLOCATOR(type)      AZ_CLASS_ALLOCATOR(type, Audio::AudioSystemAllocator, 0)
#define AUDIO_IMPL_CLASS_ALLOCATOR(type)        AZ_CLASS_ALLOCATOR(type, Audio::AudioImplAllocator, 0)
