/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/Backends/YAML/YamlUtils.h>
#include <AzCore/DOM/DomBackend.h>

namespace AZ::Dom
{
    class YamlBackend final : public Backend
    {
    public:
        Visitor::Result ReadFromBuffer(const char* buffer, size_t size, AZ::Dom::Lifetime lifetime, Visitor& visitor) override;
        // Visitor::Result ReadFromBufferInPlace(char* buffer, [[maybe_unused]] AZStd::optional<size_t> size, Visitor& visitor) override;
        Visitor::Result WriteToBuffer(AZStd::string& buffer, WriteCallback callback) override;
    };
} // namespace AZ::Dom
