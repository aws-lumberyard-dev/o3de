/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/Material/PhysXMaterialConfiguration.h>

namespace PhysX
{
    class EditorMaterialAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysX::EditorMaterialAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysX::EditorMaterialAsset, "{BC7B88B9-EE31-4FBF-A01E-2A93624C49D3}", AZ::Data::AssetData);

        EditorMaterialAsset() = default;
        virtual ~EditorMaterialAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        MaterialConfiguration m_materialConfiguration;
    };
} // namespace PhysX
