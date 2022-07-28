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

namespace ONNX
{
    void ONNXSystemComponent::SetPrecomputedTimingData(int totalCount, int64_t correctCount, float totalTime, float avgTime)
    {
        m_precomputedTimingData->m_totalNumberOfInferences = totalCount;
        m_precomputedTimingData->m_numberOfCorrectInferences = correctCount;
        m_precomputedTimingData->m_totalPrecomputedRuntime = totalTime;
        m_precomputedTimingData->m_averagePrecomputedRuntime = avgTime;
    }

    PrecomputedTimingData* ONNXSystemComponent::GetPrecomputedTimingData()
    {
        return m_precomputedTimingData.get();
    }

    void ONNXSystemComponent::SetPrecomputedTimingDataCuda(int totalCount, int64_t correctCount, float totalTime, float avgTime)
    {
        m_precomputedTimingDataCuda->m_totalNumberOfInferences = totalCount;
        m_precomputedTimingDataCuda->m_numberOfCorrectInferences = correctCount;
        m_precomputedTimingDataCuda->m_totalPrecomputedRuntime = totalTime;
        m_precomputedTimingDataCuda->m_averagePrecomputedRuntime = avgTime;
    }

    PrecomputedTimingData* ONNXSystemComponent::GetPrecomputedTimingDataCuda()
    {
        return m_precomputedTimingDataCuda.get();
    }

    void ONNXSystemComponent::AddTimingSample(const char* modelName, float inferenceTimeInMilliseconds)
    {
        m_timingStats.PushHistogramValue(modelName, inferenceTimeInMilliseconds, AZ::Color::CreateFromRgba(229, 56, 59, 255));
    }

    void ONNXSystemComponent::AddTimingSampleCuda(const char* modelName, float inferenceTimeInMilliseconds)
    {
        m_timingStatsCuda.PushHistogramValue(modelName, inferenceTimeInMilliseconds, AZ::Color::CreateFromRgba(56, 229, 59, 255));
    }

    void ONNXSystemComponent::OnImGuiUpdate()
    {
        if (!m_timingStats.m_show)
        {
            return;
        }

        PrecomputedTimingData* timingData;
        ONNXRequestBus::BroadcastResult(timingData, &ONNXRequestBus::Events::GetPrecomputedTimingData);

        PrecomputedTimingData* timingDataCuda;
        ONNXRequestBus::BroadcastResult(timingDataCuda, &ONNXRequestBus::Events::GetPrecomputedTimingDataCuda);

        if (ImGui::Begin("ONNX"))
        {
            if (ImGui::CollapsingHeader("MNIST (Precomputed)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("MNIST", 3))
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("Total Inference Runtime: %.2f ms", timingData->m_totalPrecomputedRuntime);
                    ImGui::TableNextColumn();
                    ImGui::Text("Average Inference Runtime: %.2f ms", timingData->m_averagePrecomputedRuntime);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Total No. Of Inferences: %d", timingData->m_totalNumberOfInferences);
                    ImGui::TableNextColumn();
                    ImGui::Text("No. Of Correct Inferences: %d", timingData->m_numberOfCorrectInferences);
                    ImGui::TableNextColumn();
                    ImGui::Text(
                        "Accuracy: %.2f%%",
                        ((float)timingData->m_numberOfCorrectInferences / (float)timingData->m_totalNumberOfInferences) * 100.0f);
                    ImGui::EndTable();
                }
            }
            m_timingStats.OnImGuiUpdate();

            if (ImGui::CollapsingHeader("MNIST CUDA (Precomputed)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
            {
                if (ImGui::BeginTable("MNIST", 3))
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("Total Inference Runtime: %.2f ms", timingDataCuda->m_totalPrecomputedRuntime);
                    ImGui::TableNextColumn();
                    ImGui::Text("Average Inference Runtime: %.2f ms", timingDataCuda->m_averagePrecomputedRuntime);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Total No. Of Inferences: %d", timingDataCuda->m_totalNumberOfInferences);
                    ImGui::TableNextColumn();
                    ImGui::Text("No. Of Correct Inferences: %d", timingDataCuda->m_numberOfCorrectInferences);
                    ImGui::TableNextColumn();
                    ImGui::Text(
                        "Accuracy: %.2f%%",
                        ((float)timingDataCuda->m_numberOfCorrectInferences / (float)timingDataCuda->m_totalNumberOfInferences) * 100.0f);
                    ImGui::EndTable();
                }
            }
            m_timingStatsCuda.OnImGuiUpdate();
        }
    }

    void ONNXSystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("ONNX"))
        {
            ImGui::MenuItem(m_timingStats.GetName(), "", &m_timingStats.m_show);
            ImGui::MenuItem(m_timingStatsCuda.GetName(), "", &m_timingStatsCuda.m_show);
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

        m_timingStats.SetName("MNIST Timing Statistics");
        m_timingStats.SetHistogramBinCount(200);

        m_timingStatsCuda.SetName("MNIST CUDA Timing Statistics");
        m_timingStatsCuda.SetHistogramBinCount(200);

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

    // The global environment and memory allocator are initialised with the system component, and are accessed via the EBus from within the model.
    // m_precomputedTimingData and m_precomputedTimingDataCuda are structs holding the test inference statistics run before the editor starts up.
    void ONNXSystemComponent::Init()
    {
        void* ptr;
        m_env = AZStd::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_VERBOSE, "test_log", OnnxLoggingFunction, ptr);
        m_allocator = AZStd::make_unique<Ort::AllocatorWithDefaultOptions>();
        m_precomputedTimingData = AZStd::make_unique<PrecomputedTimingData>();
        m_precomputedTimingDataCuda = AZStd::make_unique<PrecomputedTimingData>();
    }

    void ONNXSystemComponent::InitRuntimeMnistExamples() {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        m_mnist = AZStd::make_unique<MNIST>();
        m_mnistCuda = AZStd::make_unique<MNIST>();

        m_mnist->m_imageWidth = 28;
        m_mnistCuda->m_imageWidth = 28;

        m_mnist->m_imageHeight = 28;
        m_mnistCuda->m_imageHeight = 28;

        m_mnist->m_imageSize = m_mnist->m_imageWidth * m_mnist->m_imageHeight;
        m_mnistCuda->m_imageSize = m_mnistCuda->m_imageWidth * m_mnistCuda->m_imageHeight;

        std::vector<float> input(m_mnist->m_imageSize);
        m_mnist->m_input = input;
        m_mnistCuda->m_input = input;

        std::vector<float> output(10);
        m_mnist->m_output = output;
        m_mnistCuda->m_output = output;

        MNIST::InitSettings modelInitSettings;
        modelInitSettings.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettings.m_outputShape = { 1, 10 };
        modelInitSettings.m_modelName = "MNIST_Fold1 (Realtime)";

        MNIST::InitSettings modelInitSettingsCuda;
        modelInitSettingsCuda.m_inputShape = { 1, 1, 28, 28 };
        modelInitSettingsCuda.m_outputShape = { 1, 10 };
        modelInitSettingsCuda.m_modelName = "MNIST_Fold1 CUDA (Realtime)";
        modelInitSettingsCuda.m_cudaEnable = true;

        m_mnist->Load(modelInitSettings);
        m_mnistCuda->Load(modelInitSettingsCuda);

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
                    m_mnistCuda->m_input[m_mnist->m_imageWidth * y + x] = 0.0f;
                }
                else
                {
                    m_mnist->m_input[m_mnist->m_imageHeight * y + x] = 1.0f;
                    m_mnistCuda->m_input[m_mnist->m_imageHeight * y + x] = 1.0f;
                }
            }
        }

        m_mnist->BusConnect();
        m_mnistCuda->BusConnect();
    }

    void ONNXSystemComponent::Activate()
    {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        InitRuntimeMnistExamples();

        RunMnistSuite(20, false);
        RunMnistSuite(20, true);
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
