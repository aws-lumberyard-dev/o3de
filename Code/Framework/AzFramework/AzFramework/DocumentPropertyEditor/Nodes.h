/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>

namespace AZ::DocumentPropertyEditor::Nodes
{
    namespace Adapter
    {
        inline const AZ::Name Name = AZ::Name::FromStringLiteral("Adapter");
    }

    namespace Row
    {
        inline const AZ::Name Name = AZ::Name::FromStringLiteral("Row");
    }

    namespace Label
    {
        inline const AZ::Name Name = AZ::Name::FromStringLiteral("Label");
    }

    namespace PropertyEditor
    {
        inline const AZ::Name Name = AZ::Name::FromStringLiteral("PropertyEditor");
        inline const AZ::Name Type = AZ::Name::FromStringLiteral("Type");
    }
} // namespace AZ::DocumentPropertyEditor::Nodes
