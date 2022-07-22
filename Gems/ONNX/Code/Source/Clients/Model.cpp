#include "Model.h"

namespace ONNX {

    void Model::Load(InitSettings& m_init_settings) {
        Ort::Env* m_env;
        ONNXRequestBus::BroadcastResult(m_env, &ONNXRequestBus::Events::GetEnv);
        m_session = Ort::Session::Session(*m_env, m_init_settings.m_modelFile.c_str(), Ort::SessionOptions{ nullptr });
        m_input = m_init_settings.m_input;
        m_inputShape = m_init_settings.m_inputShape;
        auto m_memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        m_inputTensor = Ort::Value::CreateTensor<float>(m_memory_info, m_input.data(), m_input.size(), m_inputShape.data(), m_inputShape.size());
        Ort::AllocatorWithDefaultOptions* m_allocator;
        ONNXRequestBus::BroadcastResult(m_allocator, &ONNXRequestBus::Events::GetAllocator);
        m_inputCount = m_session.GetInputCount();
        //AZ_Printf("\nONNX", " Input Count: %d", m_inputCount);
        for (size_t i = 0; i < m_inputCount; i++) {
            const char* in_name = m_session.GetInputName(i, *m_allocator);
            m_inputNames.push_back(in_name);
            //AZ_Printf("\nONNX", " Input Name %d: %s", i, in_name);
        }
        m_output = m_init_settings.m_output;
        m_outputShape = m_init_settings.m_outputShape;
        m_outputTensor = Ort::Value::CreateTensor<float>(m_memory_info, m_output.data(), m_output.size(), m_outputShape.data(), m_outputShape.size());
        m_outputCount = m_session.GetOutputCount();
        //AZ_Printf("\nONNX", " Output Count: %d", m_outputCount);
        for (size_t i = 0; i < m_outputCount; i++) {
            const char* out_name = m_session.GetOutputName(i, *m_allocator);
            m_outputNames.push_back(out_name);
            //AZ_Printf("\nONNX", " Output Name %d: %s", i, out_name);
        }
    }

    void Model::Run() {
        m_session.Run(Ort::RunOptions{ nullptr }, m_inputNames.data(), &m_inputTensor, m_inputCount, m_outputNames.data(), &m_outputTensor, m_outputCount);
    }
}
