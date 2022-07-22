#pragma once

#include "Model.h"

namespace ONNX {
    struct MNIST : public Model
    {
        template<typename T>
        static void softmax(T& input);

        std::ptrdiff_t GetResult();

        int64_t m_result{ 0 };
    };

    void RunMnistSuite();
}
