/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Brushes/PaintBrush.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>

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
                ->Field("Opacity", &PaintBrush::m_opacity)
                ;

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
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PaintBrush::m_opacity, "Opacity", "Opacity of the paint brush.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ;
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

    void PaintBrush::Activate(AZ::EntityComponentIdPair entityComponentIdPair)
    {
        PaintBrushRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    void PaintBrush::Deactivate()
    {
        PaintBrushRequestBus::Handler::BusDisconnect();
    }
} // namespace AzToolsFramework
