/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <c4/substr.hpp>

namespace c4
{
    inline static c4::substr to_substr(AZStd::string& s)
    {
        char* data = !s.empty() ? &s[0] : nullptr;
        return c4::substr(data, s.size());
    }

    inline static c4::csubstr to_csubstr(AZStd::string const& s)
    {
        const char* data = !s.empty() ? &s[0] : nullptr;
        return c4::csubstr(data, s.size());
    }
} // namespace c4

#include <ryml.hpp>
