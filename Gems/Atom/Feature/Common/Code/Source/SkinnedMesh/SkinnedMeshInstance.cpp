/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>

namespace AZ
{
    namespace Render
    {
        void SkinnedMeshInstance::SuppressSignalOnDeallocate()
        {
            for (auto& vectorOfAllocations : m_allocations)
            {
                for (auto& allocationPtr : vectorOfAllocations)
                {
                    if (allocationPtr)
                    {
                        allocationPtr->SuppressSignalOnDeallocate();
                    }
                }
            }
        }
        
        void SkinnedMeshInstance::SetShouldSkipSkinning(uint32_t lodIndex, uint32_t meshIndex, bool shouldSkipSkinning)
        {
            m_shouldSkipSkinning[lodIndex][meshIndex] = shouldSkipSkinning;
        }
        
        bool SkinnedMeshInstance::GetShouldSkipSkinning(uint32_t lodIndex, uint32_t meshIndex) const
        {
            return m_shouldSkipSkinning[lodIndex][meshIndex];
        }
    } // namespace Render
}// namespace AZ
