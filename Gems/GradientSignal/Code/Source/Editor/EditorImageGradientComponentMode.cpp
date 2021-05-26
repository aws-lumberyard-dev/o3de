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
#include "EditorImageGradientComponentMode.h"
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        AZ::Vector3 coordinateExample{0.0f, 0.0f, 0.0f};
        ImageGradientRequestBus::Event(entityComponentIdPair.GetEntityId(), &ImageGradientRequestBus::Events::SetValue, coordinateExample, 1.0);
    }
} // namespace GradientSignal
