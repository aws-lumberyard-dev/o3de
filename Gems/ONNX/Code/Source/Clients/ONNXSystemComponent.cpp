/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ONNXSystemComponent.h"

#include "Mnist.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

// This is the structure to interface with the MNIST model
// After instantiation, set the input_image_ data to be the 28x28 pixel image of the number to recognize
// Then call Run() to fill in the results_ data with the probabilities of each
// m_result holds the index with highest probability (aka the number the model thinks is in the image)
namespace ONNX
{
    void ONNXSystemComponent::AddTimingSample(const char* modelName, float inferenceTimeInMilliseconds)
    {
        m_timingStats.PushHistogramValue(modelName, inferenceTimeInMilliseconds, AZ::Color::CreateFromRgba(229, 56, 59, 255));
    }

    void ONNXSystemComponent::OnImGuiUpdate()
    {
        if (!m_timingStats.m_show)
        {
            return;
        }

        if (ImGui::Begin("ONNX"))
        {
            if (ImGui::CollapsingHeader("Mnist Example", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("Mnist", 2))
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("Total Inference Runtime: %.2f ms", 456.0f);
                    ImGui::TableNextColumn();
                    ImGui::Text("Average Inference Runtime: %.2f ms", 0.1987f);
                    ImGui::EndTable();
                }
            }

            m_timingStats.OnImGuiUpdate();
        }
    }

    void ONNXSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("ONNX"))
        {
            ImGui::MenuItem(m_timingStats.GetName(), "", &m_timingStats.m_show);
            ImGui::EndMenu();
        }
    }

    void ONNXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ONNXSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ONNXSystemComponent>("ONNX", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
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

        m_timingStats.SetName("Timing Statistics");
        m_timingStats.SetHistogramBinCount(200);

        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
    }

    ONNXSystemComponent::~ONNXSystemComponent()
    {
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();

        if (ONNXInterface::Get() == this)
        {
            ONNXInterface::Unregister(this);
        }
    }

    Ort::Env* ONNXSystemComponent::GetEnv()
    {
        return m_env.get();
    }

    Ort::AllocatorWithDefaultOptions* ONNXSystemComponent::GetAllocator()
    {
        return m_allocator.get();
    }

    void OnnxLoggingFunction(
        void*, OrtLoggingLevel, const char* category, const char* logid, const char* code_location, const char* message)
    {
        AZ_Printf("\nONNX", "%s %s %s %s", category, logid, code_location, message);
    }

    void ONNXSystemComponent::Init()
    {
        void* ptr;
        m_env = AZStd::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_VERBOSE, "test_log", OnnxLoggingFunction, ptr);
        m_allocator = AZStd::make_unique<Ort::AllocatorWithDefaultOptions>();
    }

    void ONNXSystemComponent::Activate()
    {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        m_mnist = AZStd::make_unique<MNIST>();

        m_mnist->m_imageWidth = 28;
        m_mnist->m_imageHeight = 28;
        m_mnist->m_imageSize = m_mnist->m_imageWidth * m_mnist->m_imageHeight;
        std::vector<float> input(m_mnist->m_imageSize);
        m_mnist->m_input = input;
        std::vector<float> output(10);
        m_mnist->m_output = output;

        MNIST::InitSettings modelInitSettings;
        modelInitSettings.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettings.m_outputShape = { 1, 10 };
        modelInitSettings.m_modelName = "";

        m_mnist->Load(modelInitSettings);
        upng_t* upng = upng_new_from_file("C:/Users/kubciu/dev/o3de/Gems/ONNX/Assets/testing/3/30.png");
        upng_decode(upng);
        const unsigned char* buffer = upng_get_buffer(upng);

        for (int y = 0; y < m_mnist->m_imageHeight; y++)
        {
            for (int x = 0; x < m_mnist->m_imageWidth; x++)
            {
                int content = static_cast<int>(buffer[(y)*m_mnist->m_imageWidth + x]);
                if (content == 0)
                {
                    m_mnist->m_input[m_mnist->m_imageWidth * y + x] = 0.0f;
                }
                else
                {
                    m_mnist->m_input[m_mnist->m_imageHeight * y + x] = 1.0f;
                }
            }
        }

        m_mnist->BusConnect();

        RunMnistSuite();
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
