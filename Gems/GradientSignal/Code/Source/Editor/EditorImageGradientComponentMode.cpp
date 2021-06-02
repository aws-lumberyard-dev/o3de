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

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
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

        const AZ::Color manipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 0.6f);
        const float manipulatorRadius = 2.0f;
        const float manipulatorWidth = 0.05f;

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_angularManipulator = AzToolsFramework::AngularManipulator::MakeShared(worldFromLocal);
        m_angularManipulator->SetAxis(AZ::Vector3::CreateAxisZ());
        Refresh();

        m_angularManipulator->SetView(AzToolsFramework::CreateManipulatorViewCircle(
            *m_angularManipulator, manipulatorColor, manipulatorRadius, manipulatorWidth,
            AzToolsFramework::DrawFullCircle));

        m_angularManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }
} // namespace GradientSignal
