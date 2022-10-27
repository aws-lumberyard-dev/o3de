/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Qt
#include <QString>

// Landscape Canvas
#include <Editor/Nodes/GradientModifiers/BaseGradientModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class RidgedGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RidgedGradientModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(RidgedGradientModifierNode, "{9DC1D0D0-879E-4668-829B-96084C7175EA}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        RidgedGradientModifierNode() = default;
        explicit RidgedGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
