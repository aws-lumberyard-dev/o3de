#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace AtomSceneStream
    {

        class Umbra::AssetLoad;

        class AtomSceneStreamFeatureProcessor final
            : public RPI::FeatureProcessor
            , private AZ::TickBus::Handler
        {
        public:
            AZ_RTTI(AZ::AtomSceneStreamFeatureProcessor, "{04AF8DF3-CF8B-478B-B52A-050B7161844D}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            AtomSceneStreamFeatureProcessor();
            virtual ~AtomSceneStreamFeatureProcessor();

            bool Init(RPI::RenderPipeline* pipeline);
            bool IsInitialized() { return m_initialized; }

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;


            Data::Instance<RPI::StreamingImage> CreateStreamingTexture(Umbra::AssetLoad& job, uint32_t textureUsage);
            bool CreateStreamingMaterial(Umbra::AssetLoad& job);

        private:
            AZ_DISABLE_COPY_MOVE(AtomSceneStreamFeatureProcessor);

            const uint32_t backBuffersAmount = 3;
            AZStd::unordered_map<uint32_t, std::vector<uint8_t>> m_texturesData[backBuffersAmount]; // memory array containing K vectors where K = swap buffers amount

            static uint32_t s_instanceCount;
            uint32_t m_currentFrame = 0;  // for keeping cyclic order of allocated memory.
        };
    } // namespace AtomSceneStream
} // namespace AZ
