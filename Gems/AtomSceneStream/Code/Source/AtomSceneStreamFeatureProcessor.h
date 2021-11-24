#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <Atom/RPI.Public/FeatureProcessor.h>

#include <umbra/Client.hpp>
#include <umbra/Runtime.hpp>

namespace Umbra
{
    class AssetLoad;
}

namespace Render
{
    class MeshFeatureProcessorInterface;
}

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamFeatureProcessor final
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_RTTI(AtomSceneStreamFeatureProcessor, "{04AF8DF3-CF8B-478B-B52A-050B7161844D}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            AtomSceneStreamFeatureProcessor();
            virtual ~AtomSceneStreamFeatureProcessor();

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // RPI::SceneNotificationBus overrides ...
//            void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
//            void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
//            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

            // Umbra driven functionality
            void CleanResource();
            void UpdateStreamingResources();
            bool RestartUmbraClient();
            bool LoadStreamedAssets();
            bool UnloadStreamedAssets();
            void HandleAssetsStreaming(float seconds);

        private:
            AZ_DISABLE_COPY_MOVE(AtomSceneStreamFeatureProcessor);

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;

            Umbra::EnvironmentInfo m_env;
            Umbra::Client* m_client = nullptr;
            Umbra::Runtime* m_runtime = nullptr;
            Umbra::Scene m_scene;
            Umbra::View m_view;

            bool m_isConnectedAndStreaming = false;
            /*
            const uint32_t backBuffersAmount = 3;

            // We should not need to manage texture or any other buffer memory as it is done
            // internally by the Umbra streamer.
            AZStd::unordered_map<uint32_t, std::vector<uint8_t>> m_texturesData[backBuffersAmount]; // memory array containing K vectors where K = swap buffers amount

            static uint32_t s_instanceCount;
            uint32_t m_currentFrame = 0;  // for keeping cyclic order of allocated memory.
            */
        };
    } // namespace AtomSceneStream
} // namespace AZ
