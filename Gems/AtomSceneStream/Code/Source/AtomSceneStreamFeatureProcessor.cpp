#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/EventTrace.h>

//#pragma warning(disable:2220)

#include <Eigen/Core>
#include <Eigen/Geometry>

/*
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
*/

#include <AtomSceneStreamAssets.h>
#include <AtomSceneStreamFeatureProcessor.h>

#pragma optimize("", off)

namespace AZ
{
    namespace AtomSceneStream
    {
//        uint32_t AtomSceneStreamFeatureProcessor::s_instanceCount = 0;

        AtomSceneStreamFeatureProcessor::AtomSceneStreamFeatureProcessor()
        {
//            ++s_instanceCount;

            bool result = RestartUmbraClient();
        }

        bool AtomSceneStreamFeatureProcessor::RestartUmbraClient()
        {
            const char* apiKey = "";   // in our case should be empty string as per the mail
            // Replace the following with data from a component
            const char* locator = "v=1&project=atom%40amazon.com&model=874639baa2218cf4f35019d0dca45c1f4a3435e4&version=default&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";

            // Create a Client
            m_client = new Umbra::Client("RuntimeSample");

            // Create Umbra runtime

            UmbraEnvironmentInfoDefaults(&m_env);
            // Texture support flags is the set of textures that are supported. Runtime attempts to deliver only textures
            // that are supported.
            m_env.textureSupportFlags = UmbraTextureSupportFlags_BC1 | UmbraTextureSupportFlags_BC3 |
                UmbraTextureSupportFlags_BC5 | UmbraTextureSupportFlags_Float;

            m_runtime = new Umbra::Runtime(**m_client, m_env);
            if (!m_runtime)
            {
                AZ_Error("AtomSceneStream", false, "Error creating Umbra run time");
                return false;
            }

            // Create a Scene by connecting it to a model
            m_scene = m_runtime->createScene(apiKey, locator);

            // Wait for connection so that camera can be initialized
            for (;;)
            {
                Umbra::ConnectionStatus s = m_scene.getConnectionStatus();
                if (s == UmbraConnectionStatus_Connected)
                    break;
                else if (s == UmbraConnectionStatus_ConnectionError)
                {
                    AZ_Error("AtomSceneStream", false, "Error connecting to Umbra Back end");
                    return false;
                }
            }

            // Create a View
            m_view = m_runtime->createView();
            return true;
        }

        AtomSceneStreamFeatureProcessor::~AtomSceneStreamFeatureProcessor()
        {
            // Destroy Umbra handles
            m_view.destroy();
            m_scene.destroy();
            m_runtime->destroy();
            m_client->destroy();

            delete m_runtime;
            delete m_client;
        }

        void AtomSceneStreamFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<AtomSceneStreamFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void AtomSceneStreamFeatureProcessor::Activate()
        {
            EnableSceneNotification();
            TickBus::Handler::BusConnect();
        }

        void AtomSceneStreamFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        void AtomSceneStreamFeatureProcessor::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
        }

        int AtomSceneStreamFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void AtomSceneStreamFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (!m_runtime)
                return;

//            ++m_currentFrame;
            // run the streaming load for 10 msec - ideally this should be done on another thread!
            HandleAssetsStreaming(0.01f);   

            /*
                m_originalPipeline->GetDefaultView()

                // Update View and Runtime with new camera

                Eigen::Matrix4f worldToClip = camera.getProjection() * camera.getCameraMatrix();
                UmbraFloat4_4 umbraWorldToClip = toUmbra(worldToClip);
                UmbraFloat3 umbraCamera = toUmbra(camera.getCameraPosition());

                Umbra::ViewInfo viewInfo;
                viewInfo.cameraWorldToClip = &umbraWorldToClip;
                viewInfo.cameraPosition = &umbraCamera;
                viewInfo.cameraDepthRange = UmbraDepthRange_MinusOneToOne;
                viewInfo.cameraMatrixFormat = UmbraMatrixFormat_ColumnMajor;
                viewInfo.quality = g_quality;
                view.setCamera(viewInfo);
            */

                        // Render all objects in the View in batches
            /*
            for (;;)
            {
                Umbra::Renderable batch[128];
                int num = view.nextRenderables(batch, sizeof(batch) / sizeof(batch[0]));
                if (!num)
                    break;

                for (int i = 0; i < num; i++)
                {
                    Eigen::Matrix4f transform = Eigen::Map<Eigen::Matrix4f>(&batch[i].transform.v[0].v[0]);

                    // Color this mesh based on LOD level if requested
                    Eigen::Vector4f color = Eigen::Vector4f::Zero();
                    if (debugColors)
                    {
                        int level = batch[i].lodLevel;
                        color = getLODColor(level).homogeneous();
                    }

                    m_meshHandle = GetMeshFeatureProcessor()->AcquireMesh(AZ::Render::MeshHandleDescriptor{ meshAsset }, AZ::RPI::Material::FindOrCreate(materialAsset));
                    const AZ::Vector3 nonUniformScale(1.0f, 1.0f, 1.0f);
                    GetMeshFeatureProcessor()->SetTransform(m_meshHandle, transform, nonUniformScale);

                    renderMesh(
                        camera.getCameraPosition(),
                        camera.getProjection(),
                        transform * camera.getCameraMatrix(),
                        transform.topLeftCorner<3, 3>(),
                        color,
                        (const Mesh*)batch[i].mesh);
                }
            }
            */
            m_runtime->update();

            AZ_UNUSED(packet);
        }

        void AtomSceneStreamFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);
        }

        void AtomSceneStreamFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
        }

        void AtomSceneStreamFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
        }

        bool AtomSceneStreamFeatureProcessor::Init([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
        }


        // AssetLoad tells that an asset should be loaded into GPU. This function loads a single asset
        bool AtomSceneStreamFeatureProcessor::LoadStreamedAssets()
        {
            if (!m_runtime)
                return false;

            Umbra::AssetLoad assetLoad = m_runtime->getNextAssetLoad();
            if (!assetLoad)
                return false;

            // If memory usage is too high, the job is finished with OutOfMemory. This tells Umbra to stop loading more. The
            // quality must be reduced so that loading can continue
            /*
            if (g_gpuMemoryUsage > GPU_MEMORY_LIMIT)
            {
                m_modelsQuality *= 0.875f;
                assetLoad.finish(UmbraAssetLoadResult_OutOfMemory);
                return false;
            }
            */

            void* ptr = nullptr;

            switch (assetLoad.getType())
            {
            case UmbraAssetType_Material: ptr = new AtomSceneStream::Material(assetLoad); break;
            case UmbraAssetType_Texture: ptr = new AtomSceneStream::Texture(assetLoad); break;
            case UmbraAssetType_Mesh: ptr = new AtomSceneStream::Mesh(assetLoad); break;
            default: break;
            }

            assetLoad.prepare((Umbra::UserPointer)ptr);

            // Finish the job
            assetLoad.finish(UmbraAssetLoadResult_Success);

            return true;
        }

        // AssetUnloadJob tells that an asset can be freed from the GPU. This function frees a single asset
        bool AtomSceneStreamFeatureProcessor::UnloadStreamedAssets()
        {
            if (!m_runtime)
                return;

            Umbra::AssetUnload assetUnload = m_runtime->getNextAssetUnload();
            if (!assetUnload)
                return false;

            switch (assetUnload.getType())
            {
            case UmbraAssetType_Material: delete (AtomSceneStream::Material*)assetUnload.getUserPointer(); break;
            case UmbraAssetType_Texture: delete (AtomSceneStream::Texture*)assetUnload.getUserPointer(); break;
            case UmbraAssetType_Mesh: delete (AtomSceneStream::Mesh*)assetUnload.getUserPointer(); break;
            default: break;
            }

            assetUnload.finish();
            return true;
        }

        // Perform asset streaming work until time budget is reached
        void AtomSceneStreamFeatureProcessor::HandleAssetsStreaming(float seconds)
        {
            if (!m_runtime)
                return;

//            AZ::u64 endTime = AZStd::GetTimeUTCMilliSecond() + seconds * 1000.0f;
            auto startTime = AZStd::chrono::system_clock::now();
            auto endTime = startTime + AZStd::chrono::seconds(seconds);

            // First unload old assets to make room for new ones
//            while (AZStd::GetTimeUTCMilliSecond() () <= endTime)
            while (AZStd::chrono::system_clock::now() < endTime)
                if (!UnloadStreamedAssets())
                    break;

//            while (AZStd::GetTimeUTCMilliSecond() () <= endTime)
            while (AZStd::chrono::system_clock::now() < endTime)
                if (!LoadStreamedAssets())
                    break;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
