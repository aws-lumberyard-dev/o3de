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
#include <AzCore/Yaml/ryml.h>

namespace AZ::Dom::Yaml
{
    Visitor::Result VisitSerializedYaml(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor);
    AZStd::unique_ptr<Visitor> CreateYamlBufferWriter(AZStd::string& buffer);

} // namespace AZ::Dom::Yaml
