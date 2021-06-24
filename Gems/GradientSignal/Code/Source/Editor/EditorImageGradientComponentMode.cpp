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
#include <AzToolsFramework/Manipulators/BrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <GradientSignal/Ebuses/PaintBrushRequestBus.h>
#include <GradientSignal/Ebuses/PaintBrushNotificationBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        PaintBrushNotificationBus::Handler::BusConnect();
        PaintBrushRequestBus::EventResult(m_radius, GetEntityComponentIdPair(), &PaintBrushRequestBus::Events::GetRadius);

        const AZ::Color manipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        const float manipulatorWidth = 0.05f;

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::BrushManipulator::MakeShared(worldFromLocal);
        Refresh();

        m_brushManipulator->SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*m_brushManipulator, manipulatorColor, m_radius, manipulatorWidth));

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();
    }

    void EditorImageGradientComponentMode::HandlePaintArea(const AZ::Vector3& center)
    {
        if (m_isPainting)
        {
            float intensity = 1.0f;
            float opacity = 1.0f;
            PaintBrushRequestBus::EventResult(intensity, GetEntityComponentIdPair(), &PaintBrushRequestBus::Events::GetIntensity);
            PaintBrushRequestBus::EventResult(opacity, GetEntityComponentIdPair(), &PaintBrushRequestBus::Events::GetOpacity);

            auto SetValue = [this, intensity, opacity](float x, float y)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(x, y, 0.0f);

                float oldValue = 0.0f;
                GradientRequestBus::EventResult(oldValue, GetEntityId(), &GradientRequestBus::Events::GetValue, params);

                float newValue = opacity * intensity + (1.0f - opacity) * oldValue;
                GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::SetValue, params, newValue);
            };

            AZ::Aabb shapeBounds;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                shapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            uint32_t imageHeight = 0;
            uint32_t imageWidth = 0;
            ImageGradientRequestBus::EventResult(imageHeight, GetEntityId(), &ImageGradientRequestBus::Events::GetImageHeight);
            ImageGradientRequestBus::EventResult(imageWidth, GetEntityId(), &ImageGradientRequestBus::Events::GetImageWidth);

            const float xStep = shapeBounds.GetXExtent() / imageWidth;
            const float yStep = shapeBounds.GetYExtent() / imageHeight;

            const float manipulatorRadiusSq = m_radius * m_radius;
            const float xCenter = center.GetX();
            const float yCenter = center.GetY();

            for (float y = yCenter - m_radius; y <= yCenter + m_radius; y += yStep)
            {
                for (float x = xCenter - m_radius; x <= xCenter + m_radius; x += xStep)
                {
                    const float xDiffSq = (x - xCenter) * (x - xCenter);
                    const float yDiffSq = (y - yCenter) * (y - yCenter);
                    if (xDiffSq + yDiffSq <= manipulatorRadiusSq)
                    {
                        SetValue(x, y);
                    }
                }
            }

            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }

    bool EditorImageGradientComponentMode::HandleMouseEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        float closestDistance = std::numeric_limits<float>::max();
        AZ::EntityId entityIdUnderCursor;
        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        auto selectionFunction = [this, &closestDistance, &entityIdUnderCursor, &mouseInteraction, &viewportId](AZ::Entity* entity)
        {
            if (AzToolsFramework::PickEntity(entity->GetId(), mouseInteraction.m_mouseInteraction, closestDistance, viewportId))
            {
                entityIdUnderCursor = entity->GetId();
            }
        };

        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, selectionFunction);

        if (entityIdUnderCursor.IsValid())
        {
            AZ::Vector3 result = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin +
                mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection * closestDistance;

            AZ::Transform space = AZ::Transform::CreateTranslation(result);
            m_brushManipulator->SetSpace(space);

            HandlePaintArea(result);

            return true;
        }

        return false;
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move)
        {
            return HandleMouseEvent(mouseInteraction);
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                m_isPainting = true;
                HandleMouseEvent(mouseInteraction);
                return true;
            }
        }
        else if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up)
        {
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
            {
                m_isPainting = false;
                return true;
            }
        }
        return false;
    }

    void EditorImageGradientComponentMode::OnRadiusChanged(const float radius)
    {
        m_radius = radius;

        //m_brushManipulator->m_manipulatorView.m_radius = radius;
        m_brushManipulator->SetRadius(radius);
    }
} // namespace GradientSignal
