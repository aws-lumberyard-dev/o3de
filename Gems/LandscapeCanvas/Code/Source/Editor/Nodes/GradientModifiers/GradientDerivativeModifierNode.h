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

    class GradientDerivativeModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientDerivativeModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientDerivativeModifierNode, "{8257E364-0A8E-4F68-839A-FC3927A1A83C}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        GradientDerivativeModifierNode() = default;
        explicit GradientDerivativeModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
