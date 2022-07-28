/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Model.h"

namespace ONNX
{
    struct MNIST
        : public Model
        , public AZ::TickBus::Handler
    {
        template<typename T>
        static void softmax(T& input);

        std::ptrdiff_t GetResult();

        int m_imageWidth;
        int m_imageHeight;
        int m_imageSize;

        std::vector<float> m_input;
        std::vector<float> m_output;
        int64_t m_result{ 0 };

        virtual void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };

    void RunMnistSuite(int testsPerDigit, bool cudaEnable);
} // namespace ONNX
