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

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        GradientSignal::GradientSampleParams params;
        params.m_position = AZ::Vector3::CreateZero();

        const float newValue = 1.0f;

        GradientRequestBus::Event(entityComponentIdPair.GetEntityId(), &GradientRequestBus::Events::SetValue, params, newValue);

        const AZ::Color manipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        const float manipulatorRadius = 2.0f;
        const float manipulatorWidth = 0.05f;

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::BrushManipulator::MakeShared(worldFromLocal);
        Refresh();

        m_brushManipulator->SetView(AzToolsFramework::CreateManipulatorViewProjectedCircle(
            *m_brushManipulator, manipulatorColor, manipulatorRadius, manipulatorWidth));

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        m_brushManipulator->Unregister();
    }

    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move)
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

                return true;
            }
        }
        return false;
    }
} // namespace GradientSignal
