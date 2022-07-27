/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Fixture.h>
#include <Mnist.h>

namespace ONNX
{
    class MnistFixture : public Fixture
    {
    public:
        void SetUp() override
        {
            Fixture::SetUp();

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
            modelInitSettings.m_modelName = "MNIST_Fold1 (Test)";

            mnist.Load(modelInitSettings);
        }
        MNIST mnist;
    };

    TEST_F(MnistFixture, ModelAccuracyGreaterThan90Percent)
    {
        RunMnistSuite(200);

        PrecomputedTimingData* timingData;
        ONNXRequestBus::BroadcastResult(timingData, &ONNXRequestBus::Events::GetPrecomputedTimingData);

        float accuracy = (float)timingData->m_numberOfCorrectInferences / (float)timingData->m_totalNumberOfInferences;

        EXPECT_GT(accuracy, 0.9f);
    }
} // namespace ONNX
