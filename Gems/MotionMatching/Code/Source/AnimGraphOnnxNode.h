/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <ONNX/Model.h>
#include "CsvSerializers.h"


namespace EMotionFX::MotionMatching
{
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphOnnxNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphOnnxNode, "{3C8097A0-7E45-40E3-9DF6-53E7D6E2BB55}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum : uint16
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum : uint16
        {
            PORTID_OUTPUT_POSE = 0
        };

        AnimGraphOnnxNode();
        ~AnimGraphOnnxNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override               { return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f); }
        bool GetCanActAsState() const override                  { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

        PoseReaderCsv poseReaderCsv;
        size_t currentFrame = 0;

        ONNX::Model m_onnxModel;

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
    };
}   // namespace EMotionFX::MotionMatching
