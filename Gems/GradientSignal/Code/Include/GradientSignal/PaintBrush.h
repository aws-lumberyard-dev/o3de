/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <GradientSignal/Ebuses/PaintBrushRequestBus.h>

namespace GradientSignal
{
    class PaintBrush : public PaintBrushRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrush, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrush, "{892AFC4A-2BF6-4D63-A405-AC93F5E4832B}");
        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityComponentIdPair entityComponentIdPair);
        void Deactivate();

        // PaintBrushRequestBus overrides
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;

    private:
        float m_radius = 2.0f;
        float m_intensity = 1.0f;
        float m_opacity = 1.0f;

        AZ::u32 OnRadiusChange() const;
    };
} // namespace GradientSignal
