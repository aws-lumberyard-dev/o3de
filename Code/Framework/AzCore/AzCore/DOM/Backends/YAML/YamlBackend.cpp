/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/YAML/YamlBackend.h>

namespace AZ::Dom
{
    Visitor::Result YamlBackend::ReadFromBuffer(const char* buffer, size_t size, AZ::Dom::Lifetime lifetime, Visitor& visitor)
    {
        return Yaml::VisitSerializedYaml({buffer, size}, lifetime, visitor);
    }

    Visitor::Result YamlBackend::WriteToBuffer(AZStd::string& buffer, WriteCallback callback)
    {
        auto writer = Yaml::CreateYamlBufferWriter(buffer);
        return callback(*writer);
    }
} // namespace AZ::Dom
