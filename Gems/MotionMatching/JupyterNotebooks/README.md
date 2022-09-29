# Machine Learning Based Animation

## Background

Once the ONNX Gem was made, the goal was to use it to recreate an animation technique called Learned Motion Matching inside the MotionMatching gem within the engine. This is a novel technique concieved by [Daniel Holden](https://theorangeduck.com/page/about) and aims to bring machine learning into the more common data-driven animation technique Motion Matching. Since this part of the project requires quite a bit of pre-requisite knowledge, I recommend you look through the following before continuing:

- My presentation, [ONNX Gem & Machine Learning Based Animation](https://broadcast.amazon.com/videos/609702) by Kuba Ciukiewicz, on Broadcast
- [Motion Matching in O3DE, a Data-Driven Animation Technique](https://www.o3de.org/blog/posts/blog-motionmatching/) by Ben Jillich
- [Introducing Learned Motion Matching](https://montreal.ubisoft.com/en/introducing-learned-motion-matching/) by Daniel Holden & Ubisoft Montreal

### **Where are all the files for this?**

Unlike the ONNX Gem itself, this work is entirely experimental and as such has not been merged with any of the repos. You can find all the code in an as-is state in the [MotionMatching gem](https://github.com/aws-lumberyard-dev/o3de/tree/animation/LMM/Gems/MotionMatching) in a branch called [**Animation/LMM**](https://github.com/aws-lumberyard-dev/o3de/tree/animation/LMM) in the [**aws-lumberyard-dev/o3de**](https://github.com/aws-lumberyard-dev/o3de) repo. The model files which are hyperlinked in this README are available on GitHub. All of the other files, as well as every single file generated and data used during my experimentation can be found at the bottom of the Confluence page [here](https://wiki.agscollab.com/display/lmbr/Machine+Learning+Based+Animation+Overview). Unfortunately there is no way of transferring the files larger than 100 MB, so you will have to generate the Motion Matching Database data using the MotionMatching gem onto your own machine.
### **What data did you work with?**
The data used to train the models consists the Motion Matching Database data, in .csv form, output by the MotionMatching gem. The data consists of 5 key files in the location above:
- Position + Rotation data (Model Space): `MotionMatchingDatabase_Poses_PosRot_ModelSpace_60Hz.csv`
- Rotation data (Model Space): `MotionMatchingDatabase_Poses_Rot_ModelSpace_60Hz.csv`
- Position + Rotation data (Local Space): `MotionMatchingDatabase_Poses_PosRot_LocalSpace_60Hz.csv`
- Rotation data (Local Space): `MotionMatchingDatabase_Poses_Rot_LocalSpace_60Hz.csv`
- Feature data: `MotionMatchingDatabase_Features_60Hz.csv`

All of the models you see in my presentation are trained using some combination of these 5 data files as input.

## Basic Model Notebooks

All of the training of the ML models as well as analysis/augmentation of the data happens in a series of [JupyterNotebooks](https://github.com/aws-lumberyard-dev/o3de/tree/animation/LMM/Gems/MotionMatching/JupyterNotebooks) in the MotionMatching gem using PyTorch.

### [FeatureAnalysis.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/FeatureAnalysis.ipynb)

Initial experimentation with the feature data, plotting the data on a heatmap and scatterplot to get a general idea of structure.

### [LearnedMotionMatching.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/LearnedMotionMatching.ipynb)

This is where the majority of the models were generated. The idea here was to create a basic Stepper model that would take the Pose (Position & Rotation) + Features from the current frame, and predict what the Pose (Rotation only) would look like in the next frame. This, as well as the various improvements to try and make the model better, is explained in more detail from [28:23 - 48:00 in my presentation](https://broadcast.amazon.com/videos/609702?seek=00h28m23s).

The general process that the notebook goes through is as follows:

1. Define a data loading helper function.
2. Import the necessary frame data into Dataframes.
3. Check that all imported files contain the same number of frames.
4. Do the same with recording data (although this wasn't really used as the models created weren't great).
5. Normalize all the data using the sklearn StandardScaler (very important step).
6. Concatenate the pose data and the features to create the input.
7. Remove the last frame from the input and the first frame from the output. This is so that the same frame number in the output corresponds to the frame after the one in the input, which is how we're trying to train the model i.e to try and predict the next frame.
8. Calculate the mean squared difference between each frame in order to find the transition points between animation clips in the database. Set a threshold value and remove all frames above this value from the inputs.
9. Initialise the tensorboard writer.
10. Define a function that constructs a model of the shape we're trying to create.
11. Define a function which exports a PyTorch model into an ONNX model.
12. Define training hyperparameters.
13. Optionally define a custom loss function. In reality none of these worked very well which is why this block is commented out. We decided to stick with the standard MSELoss function.
14. Define a class which converts input and output tensors into PyTorch datasets.
15. Train the model and print loss data during training, and a loss curve after training. Then, use the function defined previously to export the trained model as an ONNX model.
16. Load the exported model and run an inference over the entire dataset.
17. Calculate the L1 and L2 norms using normalized data for each frame, and print a graph for each.
18. Do the same as above but now using unnormalized data.
19. Load the inference results into a Dataframe.
20. Export the inferenced results into a .csv file.

The files generated from this are typically something as follows:

- ONNX Model File: [`OnnxModel_PosRotFeaturesWithHandAndHeadModel_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs6_NormalizeDataOn_Threshold10.onnx`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/OnnxModel_PosRotFeaturesWithHandAndHeadModel_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs6_NormalizeDataOn_Threshold10.onnx)
- Loss curve values:
[`OnnxModelLosses_PosRotFeaturesWithHandAndHeadModel_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs6_NormalizeDataOn_Threshold10.csv`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/OnnxModelLosses_PosRotFeaturesWithHandAndHeadModel_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs6_NormalizeDataOn_Threshold10.csv)
- Inferenced output:
`InferencedPoses_PosRotFeaturesWithHandAndHeadModelAndLocal_To_RotLocal_Normalized_NoBadFrames_Shuffled_LR1e-4_Batch16_Hidden1024_WeightDecay1e-5_Epochs3_NormalizeDataOn_Threshold11.csv`

The files above constitute what I feel is the best looking model out of all my experimentation.

## Advanced Model Notebooks

Once the above models reached a good stage, I decided to try to recreate as much of the Learned Motion Matching pipeline laid out by Daniel Holden as possible. Unfortunately due to time restrictions I wasn't able to replicate this exactly, as my implementation doesn't actually do a Motion Matching search - instead it focuses only on what happens every frame. Think of it as a more complicated 'Stepper' I had above, but this time split into 3 different models, the Projector, the Stepper, and the Decompressor.

Here's a general diagram of the pipeline I ended up with:

![LearnedMotionMatchingDiagram (1)](https://user-images.githubusercontent.com/108667365/192831694-59db6d35-d34e-46d2-94f1-c840274b8dca.png)

I recommend you review the last part of my presentation, [from 48:00 onwards](https://broadcast.amazon.com/videos/609702?seek=00h48m00s), for more context.

### [Autoencoder.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Autoencoder.ipynb)

This is the notebook where I trained the Autoencoder model, that's then split up to extract the Projector model.

The process is generally similar to [LearnedMotionMatching.ipynb](#learnedmotionmatchingipynb) above, with a few modifications:

- We're not removing the first and last frames (7), as the focus of this model is to efficiently compress a frame, not predict the next one
- Bad frame removal (8) no longer happens
- We're actually creating 2 models, an Encoder and a Decoder. This is so that the Encoder can then be exported on its own after training. When using a 2-part model in PyTorch, it is important to pass parameters for both models into the optimizer, take a look at the `optimizer = torch.optim.Adam(list(modelEncoder.parameters()) + list(modelDecoder.parameters()), lr=lr)` line in the definitions. Also, in the training code, we are now passing the input through the Encoder, and then its output through the Decoder, as seen in the line `pred_y = modelDecoder(modelEncoder(X))`
- The L1 and L2 norms are no longer calculated after training the model, as this was found of limited use.
- Note: in this notebook, there are actually 2 different sets of inputs that are defined, a training input and a testing input. This was something which I wasn't fully able to get to due to time restrictions, but basically these consist of the original Motion Matching database, but with the first 20k frames split off for testing, and the remaining frames used for training. Then, in the training code, the entire testing dataset is inferenced every epoch and the loss measured. This 2nd loss curve is then plotted below the original one, which is useful to see if the model is being overtrained, as the loss for the testing dataset should only decrease.

The loss curve for this model is below:

![Projector (1)](https://user-images.githubusercontent.com/108667365/192832077-2ac1d591-bdbc-4693-b34b-b156b8407f36.png)

The files generated from this are typically something as follows:

- ONNX Model File: [`AutoEncoderModelSpace1layer1872to117SplitModelEncoder.onnx`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/AutoEncoderModelSpace1layer1872to117SplitModelEncoder.onnx)
- Loss curve values:
[`AutoEncoderModelSpace1layer1872to117SplitModel.csv`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/AutoEncoderModelSpace1layer1872to117SplitModel.csv)
- Inferenced output:
[`InferencedFeatures_AutoEncoderModelSpace1layer1872to117SplitModelEncoder.csv`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/InferencedFeatures_AutoEncoderModelSpace1layer1872to117SplitModelEncoder.csv)

The files above constitute what I feel is the best looking model out of all my experimentation.

### [Stepper.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Stepper.ipynb)

This is the notebook that trains the stepper for the more advanced models. In essence, this stepper works in the same way as the the basic one earlier, but this time taking combined features as input, and outputting the combined features for the next frame. You can find out more info about this in the [presentation](https://broadcast.amazon.com/videos/609702?seek=00h52m54s).

This notebook is largely similar to [LearnedMotionMatching.ipynb](#learnedmotionmatchingipynb), but uses combined feature data made using the actual feature data for each frame and the output from inferencing the projector model above. It also does not include the section on L1 and L2 normalization. Also, as in the projector above, the latest version includes use of a training dataset and a testing dataset.

The loss curve for this model is below:

![Stepper (1)](https://user-images.githubusercontent.com/108667365/192832206-9781666d-0df4-4d97-8662-5d7c6c8fe8a4.png)

The files generated from this are typically something as follows:

- ONNX Model File: [`Stepper2layer194to194RELUto194Epoch20LR1e-3Batch64NoBadFramesThreshold4.onnx`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/Stepper2layer194to194RELUto194Epoch20LR1e-3Batch64NoBadFramesThreshold4.onnx)
- Loss curve values:
[`Stepper2layer194to194RELUto194Epoch20LR1e-3Batch64NoBadFramesThreshold4.csv`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/Stepper2layer194to194RELUto194Epoch20LR1e-3Batch64NoBadFramesThreshold4.csv)
- Inferenced output:
`No inferenced dataset was created for this model as it was not required for any other purpose`

The files above constitute what I feel is the best looking model out of all my experimentation.

### [Decompressor.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Decompressor.ipynb)

This is the notebook that trains the decompressor, the final step in the chain of the Learned Motion Matching models. This model takes the combined features output from the Stepper and uses them to reconstruct the full local space rotation data for the pose.

The notebook follows a similar structure to the [Autoencoder.ipynb](#autoencoderipynb), but uses different input and output data and the model is a single model, not 2 separate models combined.

The loss curve for this model is below:

![Decompressor (1)](https://user-images.githubusercontent.com/108667365/192832297-a6024000-85a1-4e7f-9edb-e71315bbaa28.png)

The files generated from this are typically something as follows:

- ONNX Model File: [`DecompressorLocalSpace2layer194to1248RELUto1248Epoch100LR1e-4Batch64.onnx`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/DecompressorLocalSpace2layer194to1248RELUto1248Epoch100LR1e-4Batch64.onnx)
- Loss curve values:
[`DecompressorLocalSpace2layer194to1248RELUto1248Epoch100LR1e-4Batch64.csv`](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/GeneratedModels/DecompressorLocalSpace2layer194to1248RELUto1248Epoch100LR1e-4Batch64.csv)
- Inferenced output:
`No inferenced dataset was created for this model as it was not required for any other purpose`

The files above constitute what I feel is the best looking model out of all my experimentation.

### [Scaling.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Scaling.ipynb)

This is just a shortcut notebook I used if I quickly needed to scale the feature data using the sklearn standard scaler for some tests.

### [Splitter.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Splitter.ipynb)

I used this notebook to split the entire dataset into two. The first 20k frames became the testing set and the remaining frames became the training set - this was the data used for the experiments mentioned above where I ran out of time.

The file names are the same as for the original data but have `Testing` or `Training` appended onto the end, e.g:

- `MotionMatchingDatabase_Poses_PosRot_ModelSpace_60Hz_Testing.csv`
- `MotionMatchingDatabase_Poses_PosRot_ModelSpace_60Hz_Training.csv`

### [Combined.ipynb](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/JupyterNotebooks/Combined.ipynb)

This is the notebook I used to inference the whole sequence of Projector -> Stepper -> Compressor outside of the engine, to generate data that could be played back in the editor.

I did not get to the point of actually generating any data using this notebook, as I was instead inferencing the models at runtime within the editor using the [ONNX Gem](https://github.com/o3de/o3de-extras/tree/onnx-experimental/Gems/ONNX).

## Changes to the MotionMatching gem

In order to get the models generated above working in real time in the editor, using the Motion Matching database, a few modifications had to be made to the MotionMatching gem.

Below is the ONNX Gem dashboard running in the Editor with the final version of my Learned Motion Matching implementation:

![LearnedMotionMatchingDashboard (1)](https://user-images.githubusercontent.com/108667365/192837331-35ab25e5-faff-4e0f-945f-5561324e90e6.png)

For videos of this running as well as videos of the models, take a look at my [presentation](https://broadcast.amazon.com/videos/609702?seek=00h28m23s).

### [Code/CMakeLists.txt](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/Code/CMakeLists.txt)

- The ONNX gem was imported as a build dependency (9-27):

```C++
# Add the MotionMatching.Static target
ly_add_target(
    NAME MotionMatching.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        motionmatching_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::EMotionFXStaticLib
            Gem::ImguiAtom.Static
            ONNX.Private.Object
)
```

### [AnimGraphOnnxNode.h/.cpp](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/Code/Source/AnimGraphOnnxNode.h)

A new AnimGraph node had to be created in order to allow playback of an animation from a .csv file.

- In the InitAfterLoading in the .cpp file, lines (**36-57**), the ONNX model is initialised and loaded from a file, and the pose is read in using the [PoseReaderCsv](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/Code/Source/CsvSerializers.h).

```C++
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
```

- In the Output in the .cpp file, lines (**73-98**), the PoseReader is used to load the pose data from the current frame into the output pose. The currentFrame index is a member variable of the AnimGraphOnnxNode, and is reset to 0 when the end of the animation is reached, so that it loops over.

```C++
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
```

### [MotionMatchingInstance.h/.cpp](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/Code/Source/MotionMatchingInstance.h)

In order to actually use the [ONNX gem](https://github.com/o3de/o3de-extras/tree/onnx-experimental/Gems/ONNX) at runtime, inferencing every frame, I had to make modifications the the actual motion matching instance in order to load the models, load the training data, fit the scalers etc.

- The use of ONNX is toggled using a #define at the top of the header file (**line 39**):

```C++
#define ONNX_ENABLED true
```

- Several ONNX-specific members are also added (**lines 141-157**):

  - The 3 ONNX models for the 3 different Learned Motion Matching components.
  - The inputs to the [Projector](#autoencoderipynb) and [Stepper](#stepperipynb).
  - The current frame number.
  - The feature matrices for the input and output (the entire training dataset needs to be loaded into feature matrices in order to fit the standard scaler, which then scales the inputs and outputs for that frame).
  - The [scalers](https://github.com/aws-lumberyard-dev/o3de/blob/animation/LMM/Gems/MotionMatching/Code/Source/FeatureMatrixStandardScaler.h) for the inputs and outputs themselves.

```C++
#ifdef ONNX_ENABLED
        ONNX::Model m_onnxModel;
        ONNX::Model m_onnxStepperModel;
        ONNX::Model m_onnxDecompressorModel;

        AZStd::vector<AZStd::vector<float>> m_onnxInput;
        AZStd::vector<AZStd::vector<float>> m_onnxStepperInput;

        size_t m_currentFrame = 0;
        FeatureMatrix m_trainingFeaturePosRotMatrix;
        FeatureMatrix m_trainingFeatureRotMatrix;
        StandardScaler m_trainingModelPosRotScaler;
        StandardScaler m_trainingLocalRotScaler;

        PoseReaderCsv m_playbackAnimationPoseReader;
        FeatureMatrix m_playbackAnimationFeatureMatrix;
#endif
```

- In the .cpp file, near the bottom of the Init (**lines 144-251**):

  - All the ONNX models are loaded from their files, and the input vectors initialized to their expected sizes.
  - The Pose Readers for the input (positions and rotations in model space), and the output (rotations in local space) are initialized to read in the data from .csv files.
  - Poses for the 2 data sets are created.
  - The Feature Matrix for the input data is resized to the dimensions of the input.
  - The data is loaded into the Feature Matrix from the .csv file frame by frame:
    - Firstly, the Pose Reader applies the pose from the current frame onto a the input pose declared earlier.
    - Then, for each joint in that pose, the MODEL SPACE TRANSFORM is obtained, and the positions and rotations are read into the Feature Matrix.
  - The input data scaler is then fit to that Feature Matrix.
  - Once this is done, the Feature Matrix data is no longer needed, so the Feature Matrix is cleared in order to save memory.
  - This exact same process is then repeated, but this time for the output data and scaler.

```C++
#ifdef ONNX_ENABLED
        ONNX::Model::InitSettings onnxProjectorInitSettings;
        onnxProjectorInitSettings.m_modelFile = "D:/AutoEncoderModelSpace1layer1872to117SplitModelEncoder.onnx";
        onnxProjectorInitSettings.m_modelName = "Projector";
        m_onnxModel.Load(onnxProjectorInitSettings);

        AZStd::vector<float> projectorOnnxInput(1872);
        m_onnxInput.push_back(projectorOnnxInput);

        ONNX::Model::InitSettings onnxStepperInitSettings;
        onnxStepperInitSettings.m_modelFile = "D:/Stepper2layer194to194RELUto194Epoch20LR1e-3Batch64NoBadFramesThreshold4.onnx";
        onnxStepperInitSettings.m_modelName = "Stepper";
        onnxStepperInitSettings.m_modelColor = AZ::Color::CreateFromRgba(3, 138, 255, 255);
        m_onnxStepperModel.Load(onnxStepperInitSettings);

        AZStd::vector<float> stepperOnnxInput(194);
        m_onnxStepperInput.push_back(stepperOnnxInput);

        ONNX::Model::InitSettings onnxDecompressorInitSettings;
        onnxDecompressorInitSettings.m_modelFile = "D:/DecompressorLocalSpace2layer194to1248RELUto1248Epoch100LR1e-4Batch64.onnx";
        onnxDecompressorInitSettings.m_modelName = "Decompressor";
        onnxDecompressorInitSettings.m_modelColor = AZ::Color::CreateFromRgba(46, 204, 113, 255);
        m_onnxDecompressorModel.Load(onnxDecompressorInitSettings);

        PoseReaderCsv trainingModelPosRotReader;
        PoseReaderCsv trainingLocalRotReader;

        PoseReaderCsv::ReadSettings readSettingsTrainingPosRot;
        readSettingsTrainingPosRot.m_readPositions = true;
        readSettingsTrainingPosRot.m_readRotations = true;

        PoseReaderCsv::ReadSettings readSettingsTrainingRot;
        readSettingsTrainingRot.m_readPositions = false;
        readSettingsTrainingRot.m_readRotations = true;

        trainingModelPosRotReader.Begin("D:/MotionMatchingDatabase_Poses_PosRot_ModelSpace_60Hz.csv", readSettingsTrainingPosRot);
        trainingLocalRotReader.Begin("D:/MotionMatchingDatabase_Poses_Rot_LocalSpace_60Hz.csv", readSettingsTrainingRot);

        Pose trainingModelPosRotPose;
        trainingModelPosRotPose.LinkToActorInstance(m_actorInstance);
        trainingModelPosRotPose.InitFromBindPose(m_actorInstance);

        Pose trainingLocalRotPose;
        trainingLocalRotPose.LinkToActorInstance(m_actorInstance);
        trainingLocalRotPose.InitFromBindPose(m_actorInstance);

        const size_t numEnabledJoints = m_actorInstance->GetNumEnabledNodes();

        m_trainingFeaturePosRotMatrix.resize(trainingModelPosRotReader.GetNumPoses(), numEnabledJoints * 9);

        for (int frame = 0; frame < trainingModelPosRotReader.GetNumPoses(); frame++)
        {
            trainingModelPosRotReader.ApplyPose(m_actorInstance, trainingModelPosRotPose, TRANSFORM_SPACE_MODEL, frame);

            for (size_t i = 0; i < numEnabledJoints; ++i)
            {
                const size_t jointIndex = m_actorInstance->GetEnabledNode(i);

                Transform modelPosRotTransform = Transform::CreateIdentity();
                modelPosRotTransform = trainingModelPosRotPose.GetModelSpaceTransform(jointIndex);

                const AZ::Vector3 modelPosition = modelPosRotTransform.m_position;
                const AZ::Quaternion modelRotation = modelPosRotTransform.m_rotation;
                AZ::Matrix3x3 modelRotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(modelRotation);
                AZ::Vector3 modelRotationMatrixBasisX = modelRotationMatrix.GetBasisX().GetNormalizedSafe();
                AZ::Vector3 modelRotationMatrixBasisY = modelRotationMatrix.GetBasisY().GetNormalizedSafe();

                m_trainingFeaturePosRotMatrix.SetVector3(frame, (i * 9), modelPosition);
                m_trainingFeaturePosRotMatrix.SetVector3(frame, (i * 9) + 3, modelRotationMatrixBasisX);
                m_trainingFeaturePosRotMatrix.SetVector3(frame, (i * 9) + 6, modelRotationMatrixBasisY);
            }
        }
        
        m_trainingModelPosRotScaler.Fit(m_trainingFeaturePosRotMatrix);

        m_trainingFeaturePosRotMatrix.Clear();

        m_trainingFeatureRotMatrix.resize(trainingLocalRotReader.GetNumPoses(), numEnabledJoints * 6);

        for (int frame = 0; frame < trainingModelPosRotReader.GetNumPoses(); frame++)
        {
            trainingLocalRotReader.ApplyPose(m_actorInstance, trainingLocalRotPose, TRANSFORM_SPACE_LOCAL, frame);

            for (size_t i = 0; i < numEnabledJoints; ++i)
            {
                const size_t jointIndex = m_actorInstance->GetEnabledNode(i);

                Transform localRotTransform = Transform::CreateIdentity();
                localRotTransform = trainingLocalRotPose.GetLocalSpaceTransform(jointIndex);

                const AZ::Quaternion localRotation = localRotTransform.m_rotation;
                AZ::Matrix3x3 localRotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(localRotation);
                AZ::Vector3 localRotationMatrixBasisX = localRotationMatrix.GetBasisX().GetNormalizedSafe();
                AZ::Vector3 localRotationMatrixBasisY = localRotationMatrix.GetBasisY().GetNormalizedSafe();

                m_trainingFeatureRotMatrix.SetVector3(frame, (i * 6), localRotationMatrixBasisX);
                m_trainingFeatureRotMatrix.SetVector3(frame, (i * 6) + 3, localRotationMatrixBasisY);
            }
        }

        m_trainingLocalRotScaler.Fit(m_trainingFeatureRotMatrix);

        m_trainingFeatureRotMatrix.Clear();
#endif
```

- In the .cpp file in the Output (**lines 408-525**):
  - The current frame data is retrieved from the FrameDatabase and loaded into the outputPose.
  - The input vector for the [Projector ONNX model](#autoencoderipynb) is filled by going through each joint index, and retrieving its MODEL SPACE TRANSFORM.
  - The [projector model](#autoencoderipynb) is run with these inputs.
  - The outputs from this model are then concatenated with the features from the Feature Matrix for that frame.
  - Those combined features are then used to run the [Stepper model](#stepperipynb).
  - The output from the [stepper model](#stepperipynb) is then used to run the [Decompressor model](#decompressoripynb).
  - The m_outputs member of the [Decompressor model](#decompressoripynb) instance is then looped over, and the results loaded in as rotations into each joint in the output pose. Note these are loaded in as LOCAL SPACE TRANSFORMS.
  - The current frame index is incremented, or reset to 0 if on the last frame so that the animation loops over itself.

```C++
#ifdef ONNX_ENABLED
        const FeatureMatrix& featureMatrix = m_data->GetFeatureMatrix();
        const FrameDatabase& frameDatabase = m_data->GetFrameDatabase();
        const Frame& currentFrame = frameDatabase.GetFrame(m_currentFrame);
        size_t featureCount = featureMatrix.cols();

        currentFrame.SamplePose(&outputPose);

        const size_t numEnabledJoints = m_actorInstance->GetNumEnabledNodes();

        size_t onnxInputVectorIndex = 0;

        for (size_t i = 0; i < numEnabledJoints; ++i)
        {
            const size_t jointIndex = m_actorInstance->GetEnabledNode(i);

            Transform modelTransform = Transform::CreateIdentity();
            modelTransform = outputPose.GetModelSpaceTransform(jointIndex);

            // Position
            AZ::Vector3 position = modelTransform.m_position;
            position = m_trainingModelPosRotScaler.Transform(position, onnxInputVectorIndex);
            m_onnxInput[0][onnxInputVectorIndex] = position.GetX();
            m_onnxInput[0][onnxInputVectorIndex+1] = position.GetY();
            m_onnxInput[0][onnxInputVectorIndex+2] = position.GetZ();

            // Rotation
            // Store rotation as the X and Y axes The Z axis can be reconstructed by the cross product of the X and Y axes.
            const AZ::Quaternion rotation = modelTransform.m_rotation;
            AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateFromQuaternion(rotation);
            AZ::Vector3 rotationMatrixBasisX = rotationMatrix.GetBasisX().GetNormalizedSafe();
            AZ::Vector3 rotationMatrixBasisY = rotationMatrix.GetBasisY().GetNormalizedSafe();
            rotationMatrixBasisX = m_trainingModelPosRotScaler.Transform(rotationMatrixBasisX, onnxInputVectorIndex + 3);
            rotationMatrixBasisY = m_trainingModelPosRotScaler.Transform(rotationMatrixBasisY, onnxInputVectorIndex + 6);
            m_onnxInput[0][onnxInputVectorIndex+3] = rotationMatrixBasisX.GetX();
            m_onnxInput[0][onnxInputVectorIndex+4] = rotationMatrixBasisX.GetY();
            m_onnxInput[0][onnxInputVectorIndex+5] = rotationMatrixBasisX.GetZ();
            m_onnxInput[0][onnxInputVectorIndex+6] = rotationMatrixBasisY.GetX();
            m_onnxInput[0][onnxInputVectorIndex+7] = rotationMatrixBasisY.GetY();
            m_onnxInput[0][onnxInputVectorIndex+8] = rotationMatrixBasisY.GetZ();

            onnxInputVectorIndex += 9;
        }

        ONNX::Model* onnxModel = &m_onnxModel;

        onnxModel->Run(m_onnxInput);

        size_t onnxStepperInputVectorIndex = 0;

        for (size_t i = 0; i < featureCount; i++)
        {
            m_onnxStepperInput[0][onnxStepperInputVectorIndex + i] = featureMatrix(m_currentFrame, i);
        }

        onnxStepperInputVectorIndex += featureCount;

        for (int i = 0; i < onnxModel->m_outputs[0].size(); i++)
        {
            m_onnxStepperInput[0][onnxStepperInputVectorIndex + i] = onnxModel->m_outputs[0][i];
        }

        ONNX::Model* onnxStepperModel = &m_onnxStepperModel;

        onnxStepperModel->Run(m_onnxStepperInput);

        ONNX::Model* onnxDecompressorModel = &m_onnxDecompressorModel;

        onnxDecompressorModel->Run(onnxStepperModel->m_outputs);

        Pose* bindPose = m_actorInstance->GetTransformData()->GetBindPose();

        size_t valueIndex = 0;
        for (size_t i = 0; i < numEnabledJoints; ++i)
        {
            const size_t jointIndex = m_actorInstance->GetEnabledNode(i);

            Transform transform = bindPose->GetLocalSpaceTransform(jointIndex);

            auto LoadVector3 = [onnxDecompressorModel](size_t& valueIndex, AZ::Vector3& outVec)
            {
                outVec.SetX(onnxDecompressorModel->m_outputs[0][valueIndex + 0]);
                outVec.SetY(onnxDecompressorModel->m_outputs[0][valueIndex + 1]);
                outVec.SetZ(onnxDecompressorModel->m_outputs[0][valueIndex + 2]);
                valueIndex += 3;
            };

            // Rotation
            // Load the X and Y axes.
            AZ::Vector3 basisX = AZ::Vector3::CreateZero();
            AZ::Vector3 basisY = AZ::Vector3::CreateZero();
            LoadVector3(valueIndex, basisX);
            LoadVector3(valueIndex, basisY);
            basisX = m_trainingLocalRotScaler.InverseTransform(basisX, (i * 6));
            basisY = m_trainingLocalRotScaler.InverseTransform(basisY, (i * 6) + 3);
            basisX.NormalizeSafe();
            basisY.NormalizeSafe();

            // Create a 3x3 rotation matrix by the X and Y axes and construct the Z-axis as the
            // cross-product of the X and Y axes.
            AZ::Matrix3x3 rotationMatrix = AZ::Matrix3x3::CreateIdentity();
            rotationMatrix.SetBasisX(basisX);
            rotationMatrix.SetBasisY(basisY);
            rotationMatrix.SetBasisZ(basisX.Cross(basisY));
            rotationMatrix.GetBasisZ().NormalizeSafe();

            // Convert the rotation matrix to a quaternion.
            transform.m_rotation = AZ::Quaternion::CreateFromMatrix3x3(rotationMatrix);

            outputPose.SetLocalSpaceTransform(jointIndex, transform);
        }

        m_currentFrame++;
        if (m_currentFrame % frameDatabase.GetNumFrames() == 0)
        {
            m_currentFrame = 0;
        }
#else
```
