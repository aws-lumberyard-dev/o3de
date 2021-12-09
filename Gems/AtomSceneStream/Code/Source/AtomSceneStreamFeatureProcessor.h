#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Component/TickBus.h>

#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>

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
        class Mesh;
        using ModelsMapByModel = AZStd::unordered_map<AtomSceneStream::Mesh*, Render::MeshFeatureProcessorInterface::MeshHandle>;
        using ModelsMapByName = AZStd::unordered_map<AZStd::string, Render::MeshFeatureProcessorInterface::MeshHandle>;
//        using ModelsMapByName = AZStd::unordered_map<AZStd::string, uint32_t>;

        const uint32_t GPU_MEMORY_LIMIT = 1024 * 1024 * 1024; // 1 GiB

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
//            void OnRenderEnd() override;

            // AZ::TickBus::Handler overrides
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr renderPipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* renderPipeline) override;
//            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

            // Umbra driven functionality
            void CleanResource();
            void RemoveAllActiveModels();
            void DebugDraw(RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh, Vector3& offset, const Color& debugColor);
            void DebugDrawMeshes(RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh, const Color& debugColor);
            void UpdateStreamingResources();
            void UpdateUmbraViewCamera();
            bool StartUmbraClient(); 
            bool RegisterMeshForRender(AtomSceneStream::Mesh* currentMesh, Transform& modelTransform);
            bool LoadStreamedAssets();
            bool UnloadStreamedAssets();
            void HandleAssetsStreaming(float seconds);

        private:
            AZ_DISABLE_COPY_MOVE(AtomSceneStreamFeatureProcessor);

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
//            ModelsMapByName m_modelsMapByName;
            ModelsMapByName m_visibleModelsMapByName;
//            ModelsMapByModel m_modelsMapByModel;
            ModelsMapByName m_hiddenModelsByName;

//            AZStd::vector<Render::MeshFeatureProcessorInterface::MeshHandle> m_meshHandles;

            float m_quality = 0.5f;     // adjusted based on memory consumption
            uint32_t m_memoryUsage = 0;

            Umbra::EnvironmentInfo m_env;
            Umbra::Client* m_client = nullptr;
            Umbra::Runtime* m_runtime = nullptr;
            Umbra::Scene m_scene;
            Umbra::View m_view;

            bool m_readyForStreaming = false;
            bool m_isConnectedAndStreaming = false;
        };
    } // namespace AtomSceneStream
} // namespace AZ
