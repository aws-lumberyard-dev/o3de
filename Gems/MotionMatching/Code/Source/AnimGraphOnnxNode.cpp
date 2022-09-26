/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "CsvSerializers.h"
#include "AnimGraphOnnxNode.h"
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/EMotionFXManager.h>

namespace EMotionFX::MotionMatching
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphOnnxNode, AnimGraphAllocator, 0)

    AnimGraphOnnxNode::AnimGraphOnnxNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    AnimGraphOnnxNode::~AnimGraphOnnxNode()
    {
    }


    bool AnimGraphOnnxNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        ONNX::Model::InitSettings onnxModelInitSettings;
        onnxModelInitSettings.m_modelFile = "D:/OnnxModel_PosRotFeaturesWithHandAndHeadModelAndLocal_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs3_NormalizeDataOn_Threshold11.onnx";
        m_onnxModel.Load(onnxModelInitSettings);

        PoseReaderCsv::ReadSettings readSettings;
        readSettings.m_readPositions = false;
        readSettings.m_readRotations = true;

        poseReaderCsv.Begin("D:/InferencedPoses_PosRotFeaturesWithHandAndHeadLocal_To_RotLocal_Unnormalized_NoBadFrames_Shuffled_LR1e-3_Batch32_Hidden512_WeightDecay1e-5_Epochs3_NormalizeDataOff_170kRecordedONLY_Threshold1-5.csv", readSettings);

        Reinit();
        return true;
    }

    // get the palette name
    const char* AnimGraphOnnxNode::GetPaletteName() const
    {
        return "AnimGraphOnnxNode";
    }


    // get the category
    AnimGraphObject::ECategory AnimGraphOnnxNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    // perform the calculations / actions
    void AnimGraphOnnxNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // get the output pose
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        poseReaderCsv.ApplyPose(animGraphInstance->GetActorInstance(), outputPose->GetPose(), ETransformSpace::TRANSFORM_SPACE_LOCAL, currentFrame);
        const size_t rootMotionNodeIndex = animGraphInstance->GetActorInstance()->GetActor()->GetMotionExtractionNodeIndex();
        outputPose->GetPose().SetLocalSpaceTransform(
            rootMotionNodeIndex,
            animGraphInstance->GetActorInstance()->GetActor()->GetBindPose()->GetLocalSpaceTransform(rootMotionNodeIndex));

        currentFrame++;        
        if (currentFrame % poseReaderCsv.GetNumPoses() == 0)
        {
            currentFrame = 0;
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    void AnimGraphOnnxNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphOnnxNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphOnnxNode>("Bind Pose", "Bind pose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX::MotionMatching
