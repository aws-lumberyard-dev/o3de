/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>

namespace Blast
{
    // DEPRECATED
    // Legacy blast material Id class used to identify the material in the collection of materials.
    // Used when converting old material asset to new one.
    // Eventually this will be removed.
    class BlastMaterialId
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialId, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(BlastMaterialId, "{BDB30505-C93E-4A83-BDD7-41027802DE0A}");

        static void Reflect(AZ::ReflectContext* context);

        AZ_DEPRECATED(static BlastMaterialId Create(),
            "BlastMaterialId has been deprecated. With new blast material asset it's not necessary. Do not use.");

        bool IsNull() const
        {
            return m_id.IsNull();
        }

        bool operator==(const BlastMaterialId& other) const
        {
            return m_id == other.m_id;
        }

        const AZ::Uuid& GetUuid() const
        {
            return m_id;
        }

    private:
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
    };

    void ReflectLegacyMaterialClasses(AZ::ReflectContext* context);

} // namespace Blast
