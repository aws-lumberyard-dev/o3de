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

#include "GradientSignal_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Ebuses/PaintBrushNotificationBus.h>
#include <GradientSignal/Ebuses/PaintBrushRequestBus.h>
#include <GradientSignal/PaintBrush.h>

namespace GradientSignal
{
    void PaintBrush::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PaintBrush>()
                ->Version(1)
                ->Field("Radius", &PaintBrush::m_radius)
                ->Field("Intensity", &PaintBrush::m_intensity)
                ->Field("Opacity", &PaintBrush::m_opacity)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<PaintBrush>("Paint Brush", "")
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
                ->Property("radius", BehaviorValueProperty(&PaintBrush::m_radius))
                ->Property("intensity", BehaviorValueProperty(&PaintBrush::m_intensity))
                ->Property("opacity", BehaviorValueProperty(&PaintBrush::m_opacity));
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

    void PaintBrush::Activate()
    {
        PaintBrushRequestBus::Handler::BusConnect(m_ownerEntityId);
    }

    void PaintBrush::Deactivate()
    {
        PaintBrushRequestBus::Handler::BusDisconnect();
    }
} // namespace GradientSignal
