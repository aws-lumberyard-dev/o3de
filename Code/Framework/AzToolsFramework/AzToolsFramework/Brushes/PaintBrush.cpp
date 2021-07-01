/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Brushes/PaintBrush.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    void PaintBrush::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PaintBrush>()
                ->Version(1)
                ->Field("Radius", &PaintBrush::m_radius)
                ->Field("Intensity", &PaintBrush::m_intensity)
                ->Field("Opacity", &PaintBrush::m_opacity);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintBrush>("Paint Brush", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_radius, "Radius", "Radius of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1024.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnRadiusChange)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_intensity, "Intensity", "Intensity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnIntensityChange)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_opacity, "Opacity", "Opacity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintBrush::OnOpacityChange);
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PaintBrush>()
                ->Constructor()
                ->Property("radius", BehaviorValueGetter(&PaintBrush::m_radius), nullptr)
                ->Property("intensity", BehaviorValueGetter(&PaintBrush::m_intensity), nullptr)
                ->Property("opacity", BehaviorValueGetter(&PaintBrush::m_opacity), nullptr);
        }
    }

    AZ::u32 PaintBrush::OnIntensityChange() const
    {
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnIntensityChanged, m_radius);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 PaintBrush::OnOpacityChange() const
    {
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnOpacityChanged, m_radius);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZ::u32 PaintBrush::OnRadiusChange() const
    {
        PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnRadiusChanged, m_radius);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    float PaintBrush::GetRadius() const
    {
        return m_radius;
    }

    float PaintBrush::GetIntensity() const
    {
        return m_intensity;
    }

    float PaintBrush::GetOpacity() const
    {
        return m_opacity;
    }

    void PaintBrush::SetRadius(float radius)
    {
        m_radius = radius;
        OnRadiusChange();
    }

    void PaintBrush::SetIntensity(float intensity)
    {
        m_intensity = intensity;
        OnIntensityChange();
    }

    void PaintBrush::SetOpacity(float opacity)
    {
        m_opacity = opacity;
        OnOpacityChange();
    }

    bool PaintBrush::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
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

    bool PaintBrush::HandleMouseEvent(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
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

        AZ::Vector3 result = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin +
            mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection * closestDistance;

        m_center = result;

        if (entityIdUnderCursor.IsValid())
        {
            AZ::Transform space = AZ::Transform::CreateTranslation(result);
            PaintBrushNotificationBus::Broadcast(&PaintBrushNotificationBus::Events::OnWorldSpaceChanged, space);

            if (m_isPainting)
            {
                PaintBrushNotificationBus::Broadcast(
                    &PaintBrushNotificationBus::Events::OnPaint, AZ::Aabb::CreateCenterRadius(result, m_radius));
            }

            return true;
        }

        return false;
    }

    void PaintBrush::GetValue(const AZ::Vector3& point, float& intensity, float& opacity, bool& isValid)
    {
        const float xDiffSq = (point.GetX() - m_center.GetX()) * (point.GetX() - m_center.GetX());
        const float yDiffSq = (point.GetY() - m_center.GetY()) * (point.GetY() - m_center.GetY());
        const float manipulatorRadiusSq = m_radius * m_radius;

        if (m_isPainting && xDiffSq + yDiffSq <= manipulatorRadiusSq)
        {
            intensity = GetIntensity();
            opacity = GetOpacity();
            isValid = true;
        }
        else
        {
            intensity = 0.0f;
            opacity = 0.0f;
            isValid = false;
        }
    }

    void PaintBrush::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        PaintBrushRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    void PaintBrush::Deactivate()
    {
        PaintBrushRequestBus::Handler::BusDisconnect();
    }
} // namespace AzToolsFramework
