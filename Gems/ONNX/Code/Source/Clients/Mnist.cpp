#include "Mnist.h"

namespace ONNX {
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

    std::ptrdiff_t MNIST::GetResult() {
        softmax(m_output);
        m_result = std::distance(m_output.begin(), std::max_element(m_output.begin(), m_output.end()));
        return m_result;
    }

    struct MnistReturnValues {
        int64_t m_inference;
        float m_runtime;
    };

    MnistReturnValues MnistExample(const char* path) {
        upng_t* upng = upng_new_from_file(path);
        upng_decode(upng);
        int width = upng_get_width(upng);
        int height = upng_get_height(upng);
        int size = upng_get_size(upng);
        const unsigned char* buffer = upng_get_buffer(upng);
        std::vector<float> mnist_input_image(size);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int content = static_cast<int>(buffer[(y)*width + x]);
                if (content == 0) {
                    mnist_input_image[height * y + x] = 0.0f;
                }
                else {
                    mnist_input_image[height * y + x] = 1.0f;
                }
                std::cout << mnist_input_image[height * y + x];
            }
            std::cout << "\n";
        }

        MNIST mnist_;
        MNIST::InitSettings init_settings;

        init_settings.m_input = mnist_input_image;
        init_settings.m_inputShape = { 1, 1, 28, 28 };
        std::vector<float> output(10);
        init_settings.m_output = output;
        init_settings.m_outputShape = { 1, 10 };

        mnist_.Load(init_settings);
        mnist_.m_timer.Stamp();
        mnist_.Run();
        float delta = 1000 * mnist_.m_timer.GetDeltaTimeInSeconds();
        //AZ_Printf("\nONNX", " Runtime: %f \n", delta);
        mnist_.GetResult();

        //for (int z = 0; z < 10; z++)
        //{
            //AZ_Printf("ONNX", " %d: %f\n", z, mnist_.m_output[z]);
        //}

        //AZ_Printf("ONNX", " Result: %d\n", mnist_.m_result);

        MnistReturnValues returnValues;
        returnValues.m_inference = mnist_.m_result;
        returnValues.m_runtime = delta;
        return(returnValues);
    }

    void RunMnistSuite() {

        int numOfEach = 2;
        int totalFiles = (numOfEach * 10);
        int64_t numOfCorrectInferences = 0;
        float totalRuntimeInMilliseconds = 0;

        for (int digit = 0; digit < 10; digit++) {
            std::filesystem::directory_iterator iterator = std::filesystem::directory_iterator{ "C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/testing/" + std::to_string(digit) + "/" };
            for (int version = 0; version < numOfEach; version++) {
                std::string filepath = iterator->path().string();
                MnistReturnValues returnedValues = MnistExample(filepath.c_str());
                if (returnedValues.m_inference == digit) {
                    numOfCorrectInferences += 1;
                }
                totalRuntimeInMilliseconds += returnedValues.m_runtime;
                iterator++;
            }
        }

        float accuracy = ((float)numOfCorrectInferences / (float)totalFiles) * 100.0f;
        float avgRuntimeInMilliseconds = totalRuntimeInMilliseconds / (totalFiles);

        AZ_Printf("\nONNX", " Evaluated: %d  Correct: %d  Accuracy: %f%%", totalFiles, numOfCorrectInferences, accuracy);
        AZ_Printf("\nONNX", " Total Runtime: %fms  Avg Runtime: %fms", totalRuntimeInMilliseconds, avgRuntimeInMilliseconds);
    }
}
