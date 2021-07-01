/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>

namespace AzToolsFramework
{
    class PaintBrush : public PaintBrushRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrush, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrush, "{892AFC4A-2BF6-4D63-A405-AC93F5E4832B}");
        static void Reflect(AZ::ReflectContext* context);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();

        // PaintBrushRequestBus overrides...
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;

        void SetRadius(float radius) override;
        void SetIntensity(float intensity) override;
        void SetOpacity(float opacity) override;

        void GetValue(const AZ::Vector3& point, float& intensity, float& opacity, bool& isValid) override;
        bool HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

    private:
        bool HandleMouseEvent(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        AZ::EntityComponentIdPair m_ownerEntity;
        
        bool m_isPainting = false;

        AZ::Vector3 m_center;
        float m_radius = 2.0f;

        float m_intensity = 1.0f;
        float m_opacity = 1.0f;

        AZ::u32 OnIntensityChange() const;
        AZ::u32 OnOpacityChange() const;
        AZ::u32 OnRadiusChange() const;
    };
} // namespace AzToolsFramework
