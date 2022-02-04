/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace PhysXMaterialEditor
{
    struct MaterialDocumentSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(MaterialDocumentSettings, "{444C3BE5-62E6-4C6C-9DDB-167DE86A1EF1}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MaterialDocumentSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string m_defaultMaterialTypeName = "PhysX";
    };
} // namespace PhysXMaterialEditor
