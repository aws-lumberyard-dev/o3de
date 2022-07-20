#include "Model.h"

namespace ONNX {

    template<typename T>
    static void softmax(T& input)
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

    void Model::Load(InitSettings& m_init_settings) {
        Ort::Env* m_env;
        ONNXRequestBus::BroadcastResult(m_env, &ONNXRequestBus::Events::GetEnv);
        m_session = Ort::Session::Session(*m_env, m_init_settings.m_modelFile.c_str(), Ort::SessionOptions{ nullptr });
        m_input = m_init_settings.m_input;
        m_inputShape = m_init_settings.m_inputShape;
        auto m_memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        m_inputTensor = Ort::Value::CreateTensor<float>(m_memory_info, m_input.data(), m_input.size(), m_inputShape.data(), m_inputShape.size());
        m_inputCount = m_init_settings.m_inputCount;
        m_inputNames = m_init_settings.m_inputNames;
        for (int i = 0; i < m_inputCount; i++) {
            m_cachedInputNames.push_back(m_inputNames[i].c_str());
        }
        m_output = m_init_settings.m_output;
        m_outputShape = m_init_settings.m_outputShape;
        m_outputTensor = Ort::Value::CreateTensor<float>(m_memory_info, m_output.data(), m_output.size(), m_outputShape.data(), m_outputShape.size());
        m_outputCount = m_init_settings.m_outputCount;
        m_outputNames = m_init_settings.m_outputNames;
        for (int i = 0; i < m_outputCount; i++) {
            m_cachedOutputNames.push_back(m_outputNames[i].c_str());
        }
    }

    void Model::Run() {
        m_session.Run(Ort::RunOptions{ nullptr }, m_cachedInputNames.data(), &m_inputTensor, m_inputCount, m_cachedOutputNames.data(), &m_outputTensor, m_outputCount);
    }

    std::ptrdiff_t MNIST::GetResult() {
        softmax(m_output);
        m_result = std::distance(m_output.begin(), std::max_element(m_output.begin(), m_output.end()));
        return m_result;
    }

    void MnistExample() {
        upng_t* upng = upng_new_from_file("C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/rsz_twov2.png");
        upng_decode(upng);
        int width = upng_get_width(upng);
        int height = upng_get_height(upng);
        const unsigned char* buffer = upng_get_buffer(upng);
        const int bit_array_size = (width * height);
        std::vector<unsigned char> bit_buffer(bit_array_size);
        std::vector<float> mnist_input_image(bit_array_size);

        int counter = 1;
        int stage = 1;
        for (int i = 0; i < bit_array_size; i++)
        {
            int index = (stage * 8) - counter;
            bit_buffer[index] = !(((1 << (i % 8)) & (buffer[i / 8])) >> (i % 8));
            if (counter % 8 == 0)
            {
                counter = 1;
                stage += 1;
            }
            else
            {
                counter += 1;
            }
        }
        for (int j = 0; j < bit_array_size; j++)
        {
            std::cout << static_cast<int>(bit_buffer[j]);
            if ((j + 1) % width == 0)
            {
                std::cout << "\n";
            }
        }

        for (int k = 0; k < bit_array_size; k++)
        {
            mnist_input_image[bit_array_size - 1 - k] = bit_buffer[k];
        }

        MNIST mnist_;
        MNIST::InitSettings init_settings;

        init_settings.m_input = mnist_input_image;
        init_settings.m_inputShape = { 1, 1, 28, 28 };
        init_settings.m_inputNames = { "Input3" };
        init_settings.m_inputCount = 1;
        std::vector<float> output(10);
        init_settings.m_output = output;
        init_settings.m_outputShape = { 1, 10 };
        init_settings.m_outputNames = { "Plus214_Output_0" };
        init_settings.m_outputCount = 1;

        mnist_.Load(init_settings);
        mnist_.m_timer.Stamp();
        mnist_.Run();
        float delta = 1000 * mnist_.m_timer.GetDeltaTimeInSeconds();
        AZ_Printf("\nONNX", " Runtime: %f \n", delta);
        mnist_.GetResult();

        for (int z = 0; z < 10; z++)
        {
            AZ_Printf("ONNX", " %d: %f\n", z, mnist_.m_output[z]);
        }

        AZ_Printf("ONNX", " Result: %d\n", mnist_.m_result);
    }
}
