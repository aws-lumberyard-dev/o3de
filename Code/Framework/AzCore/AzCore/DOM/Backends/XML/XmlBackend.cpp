/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/XML/XmlBackend.h>
#include <AzCore/DOM/Backends/XML/XmlSerializationUtils.h>

namespace AZ::Dom
{
    Visitor::Result XmlBackend::ReadFromBuffer(const char* buffer, size_t size, [[maybe_unused]] AZ::Dom::Lifetime lifetime, Visitor& visitor)
    {
        return Xml::VisitSerializedXml({ buffer, size }, visitor);
    }

    Visitor::Result XmlBackend::ReadFromBufferInPlace(char* buffer, [[maybe_unused]] AZStd::optional<size_t> size, Visitor& visitor)
    {
        return Xml::VisitSerializedXmlInPlace(buffer, Lifetime::Persistent, visitor);
    }

    Visitor::Result XmlBackend::WriteToBuffer(AZStd::string& buffer, WriteCallback callback)
    {
        return callback(*Xml::CreateXmlBufferWriter(buffer));
    }
} // namespace AZ::Dom
