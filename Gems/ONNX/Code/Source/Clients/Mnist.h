#pragma once

#include "Model.h"

namespace ONNX
{
    struct MNIST : public Model
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
    };

    void RunMnistSuite();
} // namespace ONNX
