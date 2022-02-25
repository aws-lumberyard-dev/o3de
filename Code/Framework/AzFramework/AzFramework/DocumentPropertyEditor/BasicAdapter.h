/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class BasicAdapter : public DocumentAdapter
    {
    public:
        void SetContents(Dom::Value contents);
        Dom::Value GetContents() const override;
        void ApplyPatchFromView(const Dom::Patch& patch) override;

    private:
        Dom::Value m_value;
    };
} // namespace AZ::DocumentPropertyEditor
