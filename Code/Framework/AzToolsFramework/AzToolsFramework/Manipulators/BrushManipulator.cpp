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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Manipulators/BrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    AZStd::shared_ptr<BrushManipulator> BrushManipulator::MakeShared(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return AZStd::shared_ptr<BrushManipulator>(aznew BrushManipulator(worldFromLocal, entityComponentIdPair));
    }

    BrushManipulator::BrushManipulator(const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        SetSpace(worldFromLocal);

        float radius = 2.0f;
        PaintBrushRequestBus::EventResult(radius, entityComponentIdPair, &AzToolsFramework::PaintBrushRequestBus::Events::GetRadius);

        const AZ::Color manipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        const float manipulatorWidth = 0.05f;
        SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, manipulatorColor, radius, manipulatorWidth));
    }

    void BrushManipulator::Draw(
        const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            {GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver()}, debugDisplay, cameraState,
            mouseInteraction);
    }

    void BrushManipulator::SetView(AZStd::shared_ptr<ManipulatorViewProjectedCircle> view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void BrushManipulator::SetRadius(const float radius)
    {
        m_manipulatorView->SetRadius(radius);
    }
} // namespace AzToolsFramework
