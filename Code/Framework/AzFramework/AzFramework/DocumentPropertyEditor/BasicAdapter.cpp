
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/BasicAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    void BasicAdapter::SetContents(Dom::Value contents)
    {
        m_value = contents;
        ResetDocument();
    }

    Dom::Value BasicAdapter::GetContents() const
    {
        return m_value;
    }

    void BasicAdapter::ApplyPatchFromView(const Dom::Patch& patch)
    {
        patch.ApplyInPlace(m_value);
        SendPatchToView(patch);
    }
} // namespace AZ::DocumentPropertyEditor
