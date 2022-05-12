/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace ScriptAutomation
{
    namespace Utils
    {
        bool SupportsResizeClientArea();
        bool ResizeClientArea(uint32_t width, uint32_t height);
    }
}