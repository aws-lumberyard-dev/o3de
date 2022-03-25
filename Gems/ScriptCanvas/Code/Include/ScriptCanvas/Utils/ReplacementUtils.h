/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    // Use this as starting point for node replacement config lookup
    // Later we can replace it by autogen or disk file if necessary
    class ReplacementUtils
    {
    public:
        static NodeConfiguration GetReplacementMethodNode(const char* className, const char* methodName);
    };
} // namespace ScriptCanvas
