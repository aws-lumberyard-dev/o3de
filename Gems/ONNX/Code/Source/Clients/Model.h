#pragma once

#include <AzCore/Component/Component.h>
#include <onnxruntime_cxx_api.h>
#include <AzCore/Debug/Timer.h>

#include "upng.h"

#include <array>
#include <cmath>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <string>
#include <filesystem>

namespace ONNX {
    class Model {
    public:
        Model() {
        }

        struct InitSettings {
            std::wstring m_modelFile = L"C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/MNIST_Fold1.onnx";
            std::vector<float> m_input;
            std::vector<int64_t> m_inputShape;
            std::vector<float> m_output;
            std::vector<int64_t> m_outputShape;
        };

        void Load(InitSettings& m_init_settings);

        void Run();

        std::vector<float> m_input;
        std::vector<float> m_output;
        AZ::Debug::Timer m_timer;

    protected:

        Ort::Session m_session{ nullptr };
        Ort::Value m_inputTensor{ nullptr };
        std::vector<int64_t> m_inputShape;
        size_t m_inputCount;
        AZStd::vector<const char*> m_inputNames;
        Ort::Value m_outputTensor{ nullptr };
        std::vector<int64_t> m_outputShape;
        size_t m_outputCount;
        AZStd::vector<const char*> m_outputNames;
    };

    struct MNIST : public Model
    {
        template<typename T>
        static void softmax(T& input);

        std::ptrdiff_t GetResult();

        int64_t m_result{ 0 };
    };

    void RunMnistSuite();
}
