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

namespace ONNX {
    class Model {
    public:
        Model() {
        }

        struct InitSettings {
            std::wstring m_modelFile = L"C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/model.onnx";
            std::vector<float> m_input;
            std::vector<int64_t> m_inputShape;
            AZStd::vector<AZStd::string> m_inputNames;
            size_t m_inputCount;
            std::vector<float> m_output;
            std::vector<int64_t> m_outputShape;
            AZStd::vector<AZStd::string> m_outputNames;
            size_t m_outputCount;
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
        AZStd::vector<AZStd::string> m_inputNames;
        AZStd::vector<const char*> m_cachedInputNames;
        size_t m_inputCount;
        Ort::Value m_outputTensor{ nullptr };
        std::vector<int64_t> m_outputShape;
        AZStd::vector<AZStd::string> m_outputNames;
        AZStd::vector<const char*> m_cachedOutputNames;
        size_t m_outputCount;
    };

    template<typename T>
    static void softmax(T& input);

    struct MNIST : public Model
    {
        std::ptrdiff_t GetResult();

        int64_t m_result{ 0 };
    };

    void MnistExample();
}
