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

#include "EditorImageGradientComponentMode.h"
#include "GradientSignal_precompiled.h"

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/BrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::BrushManipulator::MakeShared(worldFromLocal, entityComponentIdPair);
        Refresh();

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool result = false;

        AzToolsFramework::PaintBrushRequestBus::EventResult(
            result, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::HandleMouseInteraction, mouseInteraction);
        return result;
    }

    void EditorImageGradientComponentMode::OnRadiusChanged(float radius)
    {
        m_brushManipulator->SetRadius(radius);
    }
    
    void EditorImageGradientComponentMode::OnWorldSpaceChanged(AZ::Transform result)
    {
        m_brushManipulator->SetSpace(result);
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea)
    {
        uint32_t imageHeight = 0;
        uint32_t imageWidth = 0;
        ImageGradientRequestBus::EventResult(imageHeight, GetEntityId(), &ImageGradientRequestBus::Events::GetImageHeight);
        ImageGradientRequestBus::EventResult(imageWidth, GetEntityId(), &ImageGradientRequestBus::Events::GetImageWidth);

        const float xStep = dirtyArea.GetXExtent() / imageWidth;
        const float yStep = dirtyArea.GetYExtent() / imageHeight;

        const AZ::Vector3 minDistances = dirtyArea.GetMin();
        const AZ::Vector3 maxDistances = dirtyArea.GetMax();

        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += yStep)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += xStep)
            {
                float intensity = 0.0f;
                float opacity = 0.0f;
                bool isValid = false;

                AZ::Vector3 point = AZ::Vector3(x, y, minDistances.GetZ());

                AzToolsFramework::PaintBrushRequestBus::Event(
                    GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::GetValue, point, intensity, opacity, isValid);
                if (isValid)
                {
                    GradientSignal::GradientSampleParams params;
                    params.m_position = point;

                    float oldValue = 0.0f;
                    GradientRequestBus::EventResult(oldValue, GetEntityId(), &GradientRequestBus::Events::GetValue, params);

                    float newValue = opacity * intensity + (1.0f - opacity) * oldValue;
                    GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::SetValue, params, newValue);
                }
            }
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
} // namespace GradientSignal
