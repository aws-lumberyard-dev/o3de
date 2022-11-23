/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/ExampleEditorState.h>

namespace AZ::Render
{
    static PassNameList CreateChildPasses()
    {
        // Effect chain for our example editor state.
        return PassNameList
        {   // Black and white effect
            AZ::Name("EditorModeDesaturationTemplate"),

            // Darkening effect
            AZ::Name("EditorModeTintTemplate")
        };
    }

    ExampleEditorState::ExampleEditorState()
        : EditorStateBase(
            EditorState::FocusMode,
            "EditorStateTutorial",
            CreateChildPasses())
    {
    }

    AzToolsFramework::EntityIdList ExampleEditorState::GetMaskedEntities() const
    {
        // For now, apply this effect to everything in the scene
        return {};
    }
} // namespace AZ::Render
