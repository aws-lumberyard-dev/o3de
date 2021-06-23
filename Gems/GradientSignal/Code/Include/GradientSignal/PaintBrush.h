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

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <GradientSignal/Ebuses/PaintBrushRequestBus.h>

namespace GradientSignal
{
    class PaintBrush : private PaintBrushRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintBrush, AZ::SystemAllocator, 0);
        AZ_RTTI(PaintBrush, "{892AFC4A-2BF6-4D63-A405-AC93F5E4832B}");
        static void Reflect(AZ::ReflectContext* context);

        void Activate();
        void Deactivate();

        AZ::EntityId m_ownerEntityId;

        float m_radius = 2.0f;
        float m_intensity = 1.0f;
        float m_opacity = 1.0f;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // PaintBrushRequestBus
        float GetRadius() const override;
        float GetIntensity() const override;
        float GetOpacity() const override;

    private:
        AZ::u32 OnRadiusChange() const;
    };
} // namespace GradientSignal
