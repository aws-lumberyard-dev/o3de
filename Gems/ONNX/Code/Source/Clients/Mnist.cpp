/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Mnist.h"

namespace ONNX
{
    template<typename T>
    static void MNIST::softmax(T& input)
    {
        float rowmax = *std::max_element(input.begin(), input.end());
        std::vector<float> y(input.size());
        float sum = 0.0f;
        for (size_t i = 0; i != input.size(); ++i)
        {
            sum += y[i] = std::exp(input[i] - rowmax);
        }
        for (size_t i = 0; i != input.size(); ++i)
        {
            input[i] = y[i] / sum;
        }
    }

    std::ptrdiff_t MNIST::GetResult()
    {
        softmax(m_output);
        m_result = std::distance(m_output.begin(), std::max_element(m_output.begin(), m_output.end()));
        return m_result;
    }

    void MNIST::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        Run(m_input, m_output);
    }

    struct MnistReturnValues
    {
        int64_t m_inference;
        float m_runtime;
    };

    MnistReturnValues MnistExample(MNIST& mnist, const char* path)
    {
        upng_t* upng = upng_new_from_file(path);
        upng_decode(upng);
        const unsigned char* buffer = upng_get_buffer(upng);

        for (int y = 0; y < mnist.m_imageHeight; y++)
        {
            for (int x = 0; x < mnist.m_imageWidth; x++)
            {
                int content = static_cast<int>(buffer[(y)*mnist.m_imageWidth + x]);
                if (content == 0)
                {
                    mnist.m_input[mnist.m_imageWidth * y + x] = 0.0f;
                }
                else
                {
                    mnist.m_input[mnist.m_imageHeight * y + x] = 1.0f;
                }
            }
        }
        AZ::Debug::Timer timer;
        timer.Stamp();
        mnist.Run(mnist.m_input, mnist.m_output);
        float delta = 1000 * timer.GetDeltaTimeInSeconds();
        mnist.GetResult();

        MnistReturnValues returnValues;
        returnValues.m_inference = mnist.m_result;
        returnValues.m_runtime = delta;
        return (returnValues);
    }

    void RunMnistSuite(int testsPerDigit, bool cudaEnable)
    {
        MNIST mnist;
        mnist.m_imageWidth = 28;
        mnist.m_imageHeight = 28;
        mnist.m_imageSize = mnist.m_imageWidth * mnist.m_imageHeight;
        std::vector<float> input(mnist.m_imageSize);
        mnist.m_input = input;
        std::vector<float> output(10);
        mnist.m_output = output;

        MNIST::InitSettings modelInitSettings;
        modelInitSettings.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettings.m_outputShape = { 1, 10 };

        if (cudaEnable)
        {
            modelInitSettings.m_modelName = "MNIST_Fold1 CUDA (Precomputed)";
            modelInitSettings.m_cudaEnable = true;
        }
        else
        {
            modelInitSettings.m_modelName = "MNIST_Fold1 (Precomputed)";
        }

        mnist.Load(modelInitSettings);

        int numOfEach = testsPerDigit;
        int totalFiles = 0;
        int64_t numOfCorrectInferences = 0;
        float totalRuntimeInMilliseconds = 0;

        for (int digit = 0; digit < 10; digit++)
        {
            std::filesystem::directory_iterator iterator =
                std::filesystem::directory_iterator{ "C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/testing/" + std::to_string(digit) + "/" };
            for (int version = 0; version < numOfEach; version++)
            {
                std::string filepath = iterator->path().string();
                MnistReturnValues returnedValues = MnistExample(mnist, filepath.c_str());
                if (returnedValues.m_runtime < 10.0f)
                {
                    if (returnedValues.m_inference == digit)
                    {
                        numOfCorrectInferences += 1;
                    }
                    totalRuntimeInMilliseconds += returnedValues.m_runtime;
                    iterator++;
                    totalFiles++;
                }
            }
        }

        float accuracy = ((float)numOfCorrectInferences / (float)totalFiles) * 100.0f;
        float avgRuntimeInMilliseconds = totalRuntimeInMilliseconds / (totalFiles);

        if (cudaEnable)
        {
            ONNXRequestBus::Broadcast(
                &ONNXRequestBus::Events::SetPrecomputedTimingDataCuda,
                totalFiles,
                numOfCorrectInferences,
                totalRuntimeInMilliseconds,
                avgRuntimeInMilliseconds);
        }
        else
        {
            ONNXRequestBus::Broadcast(
                &ONNXRequestBus::Events::SetPrecomputedTimingData,
                totalFiles,
                numOfCorrectInferences,
                totalRuntimeInMilliseconds,
                avgRuntimeInMilliseconds);
        }

        AZ_Printf("\nONNX", " Evaluated: %d  Correct: %d  Accuracy: %f%%", totalFiles, numOfCorrectInferences, accuracy);
        AZ_Printf("\nONNX", " Total Runtime: %fms  Avg Runtime: %fms", totalRuntimeInMilliseconds, avgRuntimeInMilliseconds);
    }
} // namespace ONNX
