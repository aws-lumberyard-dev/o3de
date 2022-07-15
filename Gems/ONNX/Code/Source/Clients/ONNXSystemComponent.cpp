
#include "ONNXSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Debug/Timer.h>

#include <onnxruntime_cxx_api.h>
#include "upng.h"

#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

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

// This is the structure to interface with the MNIST model
// After instantiation, set the input_image_ data to be the 28x28 pixel image of the number to recognize
// Then call Run() to fill in the results_ data with the probabilities of each
// result_ holds the index with highest probability (aka the number the model thinks is in the image)
struct MNIST
{
    MNIST()
    {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        input_tensor_ = Ort::Value::CreateTensor<float>(
            memory_info, input_image_.data(), input_image_.size(), input_shape_.data(), input_shape_.size());
        output_tensor_ = Ort::Value::CreateTensor<float>(
            memory_info, results_.data(), results_.size(), output_shape_.data(), output_shape_.size());
    }

    std::ptrdiff_t Run()
    {
        const char* input_names[] = { "Input3" };
        const char* output_names[] = { "Plus214_Output_0" };

        session_.Run(Ort::RunOptions{ nullptr }, input_names, &input_tensor_, 1, output_names, &output_tensor_, 1);
        softmax(results_);
        result_ = std::distance(results_.begin(), std::max_element(results_.begin(), results_.end()));
        return result_;
    }

    static constexpr const int width_ = 28;
    static constexpr const int height_ = 28;

    std::array<float, width_* height_> input_image_{};
    std::array<float, 10> results_{};
    int64_t result_{ 0 };

private:
    Ort::Env env;
    Ort::Session session_{ env, L"C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/model.onnx", Ort::SessionOptions{ nullptr } };

    Ort::Value input_tensor_{ nullptr };
    std::array<int64_t, 4> input_shape_{ 1, 1, width_, height_ };

    Ort::Value output_tensor_{ nullptr };
    std::array<int64_t, 2> output_shape_{ 1, 10 };
};

namespace ONNX
{
    void ONNXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ONNXSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ONNXSystemComponent>("ONNX", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ONNXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ONNXSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ONNXSystemComponent::ONNXSystemComponent()
    {
        if (ONNXInterface::Get() == nullptr)
        {
            ONNXInterface::Register(this);
        }
    }

    ONNXSystemComponent::~ONNXSystemComponent()
    {
        if (ONNXInterface::Get() == this)
        {
            ONNXInterface::Unregister(this);
        }
    }

    void ONNXSystemComponent::Init()
    {
    }

    void ONNXSystemComponent::Activate()
    {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        upng_t* upng = upng_new_from_file("C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/rsz_twov2.png");
        upng_decode(upng);
        int width = upng_get_width(upng);
        // int height = upng_get_height(upng);
        int size = upng_get_size(upng);
        // int bpp = upng_get_bpp(upng);
        // int bitdepth = upng_get_bitdepth(upng);
        // int pixelsize = upng_get_pixelsize(upng);
        // int components = upng_get_components(upng);
        // upng_format format = upng_get_format(upng);
        const unsigned char* buffer = upng_get_buffer(upng);
        const int bit_array_size = (size * 8);
        std::vector<unsigned char> bit_buffer(bit_array_size);
        // std::cout << bit_array_size << "\n";
        // std::cout << size*8 << "\n";
        // std::cout << height << "\n";
        // std::cout << size << "\n";
        // std::cout << bpp << "\n";
        // std::cout << bitdepth << "\n";
        // std::cout << pixelsize << "\n";
        // std::cout << components << "\n";
        // std::cout << format << "\n";
        // std::cout << buffer << "\n";

        int counter = 1;
        int stage = 1;
        for (int i = 0; i < size * 8; i++)
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
        for (int j = 0; j < size * 8; j++)
        {
            std::cout << static_cast<int>(bit_buffer[j]);
            if ((j + 1) % width == 0)
            {
                std::cout << "\n";
            }
        }

        MNIST mnist_;

        for (int k = 0; k < size * 8; k++)
        {
            mnist_.input_image_[(size * 8) - 1 - k] = bit_buffer[k];
        }

        mnist_.Run();

        for (int z = 0; z < 10; z++)
        {
            std::cout << z << ": " << mnist_.results_[z] << "\n";
        }

        std::cout << "Result: " << mnist_.result_ << "\n";
    }

    void ONNXSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ONNXRequestBus::Handler::BusDisconnect();
    }

    void ONNXSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace ONNX
