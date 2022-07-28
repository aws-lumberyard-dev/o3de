/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Timer.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <ONNX/ONNXBus.h>
#include <onnxruntime_cxx_api.h>

#include "upng.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

namespace ONNX
{
    class Model
    {
    public:
        Model()
        {
        }

        struct InitSettings
        {
            std::wstring m_modelFile = L"C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/MNIST_Fold1_v2.onnx";
            std::string m_modelName = "";
            std::vector<int64_t> m_inputShape;
            std::vector<int64_t> m_outputShape;
            bool m_cudaEnable = false;
        };

        void Load(InitSettings& m_init_settings);

        void Run(std::vector<float>& input, std::vector<float>& output);

        AZ::Debug::Timer m_timer;
        std::string m_modelName;

    protected:
        bool m_cudaEnable;
        Ort::MemoryInfo m_memoryInfo{ nullptr };
        Ort::Session m_session{ nullptr };
        std::vector<int64_t> m_inputShape;
        size_t m_inputCount;
        AZStd::vector<const char*> m_inputNames;
        std::vector<int64_t> m_outputShape;
        size_t m_outputCount;
        AZStd::vector<const char*> m_outputNames;
    };
} // namespace ONNX
