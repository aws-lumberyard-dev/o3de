#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Debug/Timer.h>
#include <onnxruntime_cxx_api.h>

#include "upng.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
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
            std::vector<int64_t> m_inputShape;
            std::vector<int64_t> m_outputShape;
        };

        void Load(InitSettings& m_init_settings);

        void Run(std::vector<float>& input, std::vector<float>& output);

        AZ::Debug::Timer m_timer;

    protected:
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
