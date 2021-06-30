/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Manipulators/BrushManipulator.h>

namespace GradientSignal
{
    class EditorImageGradientComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AzToolsFramework::PaintBrushNotificationBus::Handler
    {
    public:
        EditorImageGradientComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorImageGradientComponentMode() override;

        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        void Refresh() override { }

    protected:
        // PaintBrushNotificationBus overrides
        void OnRadiusChanged(float radius) override;
        void OnPaint(const AZ::Aabb& dirtyArea) override;
        void OnWorldSpaceChanged(AZ::Transform result) override;

    private:
        AZStd::shared_ptr<AzToolsFramework::BrushManipulator> m_brushManipulator;
    };
} // namespace GradientSignal
