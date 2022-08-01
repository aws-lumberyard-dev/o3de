# ONNX

This is an experimental gem implementing [ONNX Runtime](https://onnxruntime.ai/) within O3DE, and demoing it using inference examples from the MNIST dataset with an Editor dashboard displaying inference statistics. Image decoding is done using a modified version of [uPNG](https://github.com/elanthis/upng).

![Image of ONNX dashboard](Docs/Images/ImGuiDashboard.png)

## Setup

1. Download the zip of the GPU version of ONNX Runtime v1.9.0 for x64 Windows from the [ONNX Runtime Github](https://github.com/microsoft/onnxruntime/releases/tag/v1.9.0), extract, and put inside the External folder in the gem. You should have a folder named onnxruntime-win-x64-gpu-1.9.0 inside External.
2. Put your .onnx file inside the Assets folder of the gem. By default the Model wrapper class looks for a file called model.onnx. To run the MNIST example you need an MNIST model, which you can get from the [ONNX Github](https://github.com/onnx/models/tree/main/vision/classification/mnist).
3. Download Johnathan Orsolini's [MNIST.png dataset](https://www.kaggle.com/datasets/playlist/mnistzip) from Kaggle, unzip (takes a while, there are a lot of pictures), and place in Assets folder of the gem (i.e. you should have a folder named mnist_png inside Assets).
4. GPU inferencing using CUDA is ENABLED by default. Please see the below `Requirements to inference using GPU` to make sure you have the correct dependencies installed. If you do not have a CUDA enabled NVIDIA GPU, or would like to run the demo without the CUDA example, then you must make sure that line 12 in the top level CMakeLists.txt of the gem is set to `add_compile_definitions("ENABLE_CUDA=false")`.
5. Add the `ONNX` gem to your project using the [Project Manager](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/) or the [Command Line Interface (CLI)](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/#using-the-command-line-interface-cli). See the documentation on  [Adding and Removing Gems in a Project](https://docs.o3de.org/docs/user-guide/project-config/add-remove-gems/).
6. Compile your project and run.
7. Once the editor starts, and go to edit a level, you should be able to press the `HOME` (or equivalent) button on your keyboard to see the realtime inference dashboard as shown above.
    - The data shown above the graphs shows statistics for precomputed CPU inferences run before the editor starts up, on a selection of the MNIST testing images. The first (static) graph shows the individual runtimes for these inferences.
    - The data in the second graph shows the runtimes for the real-time CPU inferences being run on each game tick, which are run repeatedly on the same image.
    - If CUDA is enabled, a duplicate set of data and graphs will be shown for GPU inferences using CUDA.

## Requirements to inference using GPU:

- [CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit) v11.4 or greater.
- [CUDNN library](https://developer.nvidia.com/cudnn) v8.2.4 or greater.
- [zlib](https://zlib.net/) v1.2.3 or greater.