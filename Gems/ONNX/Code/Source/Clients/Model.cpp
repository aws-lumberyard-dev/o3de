#include "Model.h"

namespace ONNX
{

    void Model::Load(InitSettings& initSettings)
    {
        Ort::Env* m_env;
        ONNXRequestBus::BroadcastResult(m_env, &ONNXRequestBus::Events::GetEnv);
        m_session = Ort::Session::Session(*m_env, initSettings.m_modelFile.c_str(), Ort::SessionOptions{ nullptr });
        m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        Ort::AllocatorWithDefaultOptions* m_allocator;
        ONNXRequestBus::BroadcastResult(m_allocator, &ONNXRequestBus::Events::GetAllocator);
        m_inputShape = initSettings.m_inputShape;
        m_inputCount = m_session.GetInputCount();
        // AZ_Printf("\nONNX", " Input Count: %d", m_inputCount);
        for (size_t i = 0; i < m_inputCount; i++)
        {
            const char* in_name = m_session.GetInputName(i, *m_allocator);
            m_inputNames.push_back(in_name);
            // AZ_Printf("\nONNX", " Input Name %d: %s", i, in_name);
        }
        m_outputShape = initSettings.m_outputShape;
        m_outputCount = m_session.GetOutputCount();
        // AZ_Printf("\nONNX", " Output Count: %d", m_outputCount);
        for (size_t i = 0; i < m_outputCount; i++)
        {
            const char* out_name = m_session.GetOutputName(i, *m_allocator);
            m_outputNames.push_back(out_name);
            // AZ_Printf("\nONNX", " Output Name %d: %s", i, out_name);
        }
    }

    void Model::Run(std::vector<float>& input, std::vector<float>& output)
    {
        Ort::Value inputTensor =
            Ort::Value::CreateTensor<float>(m_memoryInfo, input.data(), input.size(), m_inputShape.data(), m_inputShape.size());
        Ort::Value outputTensor =
            Ort::Value::CreateTensor<float>(m_memoryInfo, output.data(), output.size(), m_outputShape.data(), m_outputShape.size());
        m_session.Run(
            Ort::RunOptions{ nullptr },
            m_inputNames.data(),
            &inputTensor,
            m_inputCount,
            m_outputNames.data(),
            &outputTensor,
            m_outputCount);
    }
} // namespace ONNX
