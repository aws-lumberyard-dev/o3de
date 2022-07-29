# ONNX Gem
This is an experimental gem implementing [ONNX Runtime](https://onnxruntime.ai/) within O3DE, and demoing it using inference examples from the MNIST dataset.

![Image of ONNX dashboard](/Docs/Images/ImGuiDashboard.png)

## 3rd party requirements:
- Download the zip of the GPU version of ONNX Runtime v1.9.0 for x64 Windows from the (ONNX Runtime Github)[https://github.com/microsoft/onnxruntime/releases/tag/v1.9.0], extract, and put inside the External folder in the gem. You should have a folder named onnxruntime-win-x64-gpu-1.9.0 inside External.
- Put your .onnx file inside the Assets folder of the gem. By default the Model wrapper class looks for a file called model.onnx. To run the MNIST example you need an MNIST model, which you can get from the (ONNX Github)[https://github.com/onnx/models/tree/main/vision/classification/mnist].
- Download Johnathan Orsolini's [MNIST.png dataset](https://www.kaggle.com/datasets/playlist/mnistzip) from Kaggle, unzip (takes a while, there are a lot of pictures), and place in Assets folder of the gem (i.e. you should have a folder named mnist_png inside Assets).

## Requirements to inference using GPU:
- [CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit) v11.4 or greater.
- [CUDNN library](https://developer.nvidia.com/cudnn) v8.2.4 or greater.
- [zlib](https://zlib.net/) v1.2.3 or greater.