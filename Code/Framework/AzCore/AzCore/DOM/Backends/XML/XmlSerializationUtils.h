/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/DomVisitor.h>

namespace AZ::Dom::Xml
{
    Visitor::Result VisitSerializedXml(AZStd::string_view buffer, Visitor& visitor);
    Visitor::Result VisitSerializedXmlInPlace(char* buffer, Lifetime lifetime, Visitor& visitor);
    AZStd::unique_ptr<Visitor> CreateXmlBufferWriter(AZStd::string& buffer);
}
