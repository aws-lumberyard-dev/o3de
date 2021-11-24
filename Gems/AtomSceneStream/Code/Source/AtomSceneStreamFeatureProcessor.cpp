#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>

#include <AzFramework/Components/CameraBus.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>


//#include <Eigen/Core>
//#include <Eigen/Geometry>

#include <AtomSceneStreamAssets.h>
#include <AtomSceneStreamFeatureProcessor.h>

#pragma optimize("", off)

#ifndef SAFE_DELETE
    #define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
#endif

namespace AZ
{
    namespace AtomSceneStream
    {
//        uint32_t AtomSceneStreamFeatureProcessor::s_instanceCount = 0;

        AtomSceneStreamFeatureProcessor::AtomSceneStreamFeatureProcessor()
        {
//            ++s_instanceCount;

            [[maybe_unused]]bool result = RestartUmbraClient();
        }

        void AtomSceneStreamFeatureProcessor::CleanResource()
        {
            m_view.destroy();
            m_scene.destroy();
            if (m_runtime)
            {
                m_runtime->destroy();
            }
            if (m_client)
            {
                m_client->destroy();
            }

            SAFE_DELETE(m_runtime);
            SAFE_DELETE(m_client);
        }

        bool AtomSceneStreamFeatureProcessor::RestartUmbraClient()
        {
            const char* apiKey = "";   // in our case should be empty string as per the mail
            // Replace the following with data from a component
            const char* locator = nullptr;

            [[maybe_unused]] const char* orgLocator =
                "v=1&project=atom%40amazon.com&model=874639baa2218cf4f35019d0dca45c1f4a3435e4&version=default&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";
            // Porsche model
            [[maybe_unused]] const char* porscheLocator =
                "v=1&project=atom%40amazon.com&model=porsche-918-spyder-20211119_121403&version=-991378590&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";
            //Bentley model
            [[maybe_unused]] const char* bentleyLocator =
                "v=1&project=atom%40amazon.com&model=bentley-flying-spur-2020-20211119_124412&version=-386855994&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";

            locator = porscheLocator;

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
                CleanResource();
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
                    CleanResource();
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
            CleanResource();
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

            m_meshFeatureProcessor =  GetParentScene()->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();
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

        void AtomSceneStreamFeatureProcessor::UpdateStreamingResources()
        {
            if (!m_runtime)
                return;

            //            ++m_currentFrame;
            // run the streaming load for 20 msec - ideally this should be done on another thread!
            HandleAssetsStreaming(0.025f);

            //----------------- Camera Transform Matrix ------------------
            AZ::Transform activeCameraTransform;
            Camera::ActiveCameraRequestBus::BroadcastResult(activeCameraTransform,
                &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);
            const AZ::Matrix4x4 cameraMatrix = AZ::Matrix4x4::CreateFromTransform(activeCameraTransform);

            //--------------- Camera Projection Matrix -------------------
            Camera::Configuration config;
            Camera::ActiveCameraRequestBus::BroadcastResult(config,
                &Camera::ActiveCameraRequestBus::Events::GetActiveCameraConfiguration);

            float nearDist = config.m_nearClipDistance;
            float farDist = config.m_farClipDistance;
            float aspectRatio = config.m_frustumWidth / config.m_frustumHeight;
            Matrix4x4 viewToClipMatrix;
            MakePerspectiveFovMatrixRH(viewToClipMatrix, config.m_fovRadians, aspectRatio, nearDist, farDist);


            //---------- Setting the ViewInfo for the streaming data query -----------
            // Camera position
            Vector3 cameraPos = cameraMatrix.GetTranslation();
            UmbraFloat3 umbraCamera;
            umbraCamera.v[0] = cameraPos.GetX();
            umbraCamera.v[1] = cameraPos.GetY();
            umbraCamera.v[2] = cameraPos.GetZ();

            // Camera View-Projection
            AZ::Matrix4x4 worldToClip = viewToClipMatrix * cameraMatrix;
            UmbraFloat4_4 umbraWorldToClip;
            worldToClip.StoreToColumnMajorFloat16((float*)&umbraWorldToClip.v[0]);

            // The data 
            Umbra::ViewInfo viewInfo;
            viewInfo.cameraWorldToClip = &umbraWorldToClip;
            viewInfo.cameraPosition = &umbraCamera;
            viewInfo.cameraDepthRange = UmbraDepthRange_MinusOneToOne;
            viewInfo.cameraMatrixFormat = UmbraMatrixFormat_ColumnMajor;
            viewInfo.quality = 1.0f;// g_quality;
            m_view.setCamera(viewInfo);

            /*
            * //            m_targetView = scene.GetDefaultRenderPipeline()->GetDefaultView();
            * 
            AZ::EntityId activeCameraId;
            Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequests::GetActiveCamera);
            if (activeCameraId.IsValid())
            {
            AZ::TransformBus::EventResult(cameraPosition, activeCameraId, &AZ::TransformInterface::GetWorldTranslation);
            cameraPositionIsValid = true;
            }
            */

            if (!m_meshFeatureProcessor)
            {
                m_meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();
                AZ_Warning("AtomSceneStream", false , "MeshFeatureProcessor was not acquired.");
                return;
            }

            for (;;)
            {
                Umbra::Renderable batch[128];
                int num = m_view.nextRenderables(batch, sizeof(batch) / sizeof(batch[0]));
                if (!num)
                    break;

                for (int i = 0; i < num; i++)
                {
                    //                    Eigen::Matrix4f transform = Eigen::Map<Eigen::Matrix4f>(&batch[i].transform.v[0].v[0]);
                    //
                    //                    // Color this mesh based on LOD level if requested
                    //                    Eigen::Vector4f color = Eigen::Vector4f::Zero();
                    //                    if (debugColors)
                    //                    {
                    //                        int level = batch[i].lodLevel;
                    //                        color = getLODColor(level).homogeneous();
                    //                    }

                    AtomSceneStream::Mesh* currentMesh = (AtomSceneStream::Mesh*)batch[i].mesh;
                    if (!currentMesh)
                    {
                        AZ_Warning("AtomSceneStream", false, "Missing mesh");
                        continue;
                    }

                    // Adding the model to the mesh feature processor to be rendered this frame.
                    const Render::MeshHandleDescriptor meshDescriptor = Render::MeshHandleDescriptor{ currentMesh->GetAtomModel()->GetModelAsset() };
                    Render::MeshFeatureProcessorInterface::MeshHandle meshHandle = m_meshFeatureProcessor->AcquireMesh(meshDescriptor, currentMesh->GetAtomMaterial() );

                    const float* matrixValues = (const float*)&batch[i].transform.v[0].v[0];
                    Matrix3x4 modelMatrix = Matrix3x4::CreateFromColumnMajorFloat16(matrixValues);
                    Transform modelTransform = Transform::CreateFromMatrix3x4(modelMatrix);
                    const AZ::Vector3 nonUniformScale(1.0f, 1.0f, 1.0f);
                    m_meshFeatureProcessor->SetTransform(meshHandle, modelTransform, nonUniformScale);

                    //                    renderMesh(
                    //                        camera.getCameraPosition(),
                    //                        camera.getProjection(),
                    //                        transform * camera.getCameraMatrix(),
                    //                        transform.topLeftCorner<3, 3>(),
                    //                        color,
                    //                        (const Mesh*)batch[i].mesh);
                }
            }

            m_runtime->update();
        }

        void AtomSceneStreamFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            UpdateStreamingResources();

            AZ_UNUSED(packet);
        }

        void AtomSceneStreamFeatureProcessor::Render([[maybe_unused]] const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);
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
                return false;

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
            int workTimeMilliseconds = int(seconds * 1000.0f + 0.5f);
            auto endTime = startTime + AZStd::chrono::milliseconds(workTimeMilliseconds);

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
