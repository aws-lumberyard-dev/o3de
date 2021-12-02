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
        AtomSceneStreamFeatureProcessor::AtomSceneStreamFeatureProcessor()
        {
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

        bool AtomSceneStreamFeatureProcessor::StartUmbraClient()
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

            Umbra::SceneInfo info;
            m_scene.getSceneInfo(info);
            AZ_TracePrintf("AtomSceneStream", "\n============================\nScene Info:\n\tMin = (%.2f, %.2f, %.2f)\n\tMax = (%.2f, %.2f, %.2f)\n============================\n",
                info.bounds.mn.v[0], info.bounds.mn.v[1], info.bounds.mn.v[2],
                info.bounds.mx.v[0], info.bounds.mx.v[1], info.bounds.mx.v[2]
            );

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
        }

        void AtomSceneStreamFeatureProcessor::Deactivate()
        {
            m_readyForStreaming = false;
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        void AtomSceneStreamFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
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
            UmbraFloat3 umbraCameraPos;
            umbraCameraPos.v[0] = cameraPos.GetX();
            umbraCameraPos.v[1] = cameraPos.GetY();
            umbraCameraPos.v[2] = cameraPos.GetZ();

            AZ::Matrix4x4 cameraMatrixAtomToUmbra;

            // The following did bring the model into view but the LODs did not update, hence probably
            // still off - the reason is that the rotation is carried on the local and not world coords
//            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(-90.0f)) * cameraMatrix;
//            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationZ(AZ::DegToRad(180.0f)) * cameraMatrixAtomToUmbra;

            // Correct but translation components for Y and Z are positive instead of negative
            // First remove the translation part
            /*
            cameraPos = -cameraPos;
            Matrix4x4 translationMatrix = Matrix4x4::CreateTranslation(cameraPos) - Matrix4x4::CreateIdentity();
            cameraMatrixAtomToUmbra = cameraMatrix + translationMatrix;  // remove the translation component
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(180.0f)) * cameraMatrixAtomToUmbra;
            // Once the rotation was done, add the rotated inverse translation in
            Matrix4x4 invertedMatrix = cameraMatrixAtomToUmbra;
            invertedMatrix.InvertFast();
            Vector3 umbraCameraTranslation = invertedMatrix * cameraPos;
//            Vector3 umbraCameraTranslation = cameraMatrixAtomToUmbra * cameraPos;
            cameraMatrixAtomToUmbra.SetTranslation(umbraCameraTranslation);
            
            // Inverse the position to match Umbra camera
            cameraPos = -cameraPos;
            cameraMatrixAtomToUmbra = cameraMatrix;
            cameraMatrixAtomToUmbra.SetTranslation(cameraPos);
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(180.0f)) * cameraMatrixAtomToUmbra;
            */

            cameraMatrixAtomToUmbra = cameraMatrix;
            // Zero the translation
            cameraMatrixAtomToUmbra.SetTranslation(Vector3::CreateZero());
            // Rotate the camera in local coordinates to bring to Umbra coordinates system 
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(180.0f)) * cameraMatrixAtomToUmbra;

            // Inverse and transform the position to match Umbra camera position treatment
            cameraPos = -cameraPos;
            Vector3 umbraCameraTranslation = cameraMatrixAtomToUmbra * cameraPos;
            cameraMatrixAtomToUmbra.SetTranslation(umbraCameraTranslation);

            // Should be the right way to get to Umbra camera transpose in Atom representation
//            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(180.0f)) * cameraMatrix;
//            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(90.0f)) * cameraMatrixAtomToUmbra;

            // Camera View-Projection
            AZ::Matrix4x4 worldToClip = viewToClipMatrix * cameraMatrixAtomToUmbra;
            UmbraFloat4_4 umbraWorldToClip;
//            worldToClip.StoreToRowMajorFloat16((float*)&umbraWorldToClip.v[0]);
//            worldToClip.Transpose();
            worldToClip.StoreToColumnMajorFloat16((float*)&umbraWorldToClip.v[0]);    // Atom and Umbra have transposed matrix usage (column vs' row major)
            float umbraDefaultValues[16] =
            {
                1.3f, -0.03f,   0,       0,
                0.03f,    1.73f, -0.02f, -0.02f,
                0, -0.03f, -1, -1,
                0.016f,   3.0f,     57.32f,   57.32
            };
            float umbraStartValues[16] =
            {
                1.38,    0,       0,   0,
                0,       1.72,    0,   0,
                0,       0,       -1,  -1,
                0, 1, 57.5401268, 57.5401268
            };
 //           memcpy((void*)&umbraWorldToClip.v[0], umbraStartValues, sizeof(umbraStartValues));
 //           memcpy((void*)&umbraWorldToClip.v[0], umbraDefaultValues, sizeof(umbraDefaultValues));

            // The data 
            Umbra::ViewInfo viewInfo;
            viewInfo.cameraWorldToClip = &umbraWorldToClip;
            viewInfo.cameraPosition = &umbraCameraPos;
            viewInfo.cameraDepthRange = UmbraDepthRange_MinusOneToOne;
            viewInfo.cameraMatrixFormat = UmbraMatrixFormat_ColumnMajor;
            viewInfo.quality = m_quality;
            m_view.setCamera(viewInfo);

            Render::MeshFeatureProcessorInterface* currentMeshFeatureProcessor = GetParentScene()->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();
            if (!currentMeshFeatureProcessor || (currentMeshFeatureProcessor != m_meshFeatureProcessor))
            {   // Ignore previous stored models - they need to be registered again on the current mesh feature processor
                m_modelsMap.clear();    // Do we need to clean any memory? probably not since Umbra manages it indirectly.
                m_meshFeatureProcessor = currentMeshFeatureProcessor;

                if (!m_meshFeatureProcessor)
                {
                    AZ_Warning("AtomSceneStream", false, "MeshFeatureProcessor was not acquired.");
                    return;
                }
            }

            ModelsMap currentModels;
            for (;;)
            {
                Umbra::Renderable batch[128];
                int num = m_view.nextRenderables(batch, sizeof(batch) / sizeof(batch[0]));
                if (!num)
                    break;

                for (int i = 0; i < num; i++)
                {
                    AtomSceneStream::Mesh* currentMesh = (AtomSceneStream::Mesh*)batch[i].mesh;

                    const float* matrixValues = (const float*)&batch[i].transform.v[0].v[0];
                    Matrix3x4 modelMatrix = Matrix3x4::CreateFromColumnMajorFloat16(matrixValues);
                    Transform modelTransform = Transform::CreateFromMatrix3x4(modelMatrix);

                    AZ::Transform positionTransform = AZ::Transform::CreateIdentity();
                    positionTransform.SetTranslation(modelMatrix.GetTranslation());

                    if (RegisterMeshForRender(currentMesh, positionTransform))
                    {   // add the entry to mark that the model exists in the current frame.
                        currentModels[currentMesh] = Render::MeshFeatureProcessorInterface::MeshHandle();
                    }
                }
            }

            /*
            // Remove old entries that should not be rendered anymore.
            // [remark] - this should not be required if the stream offload deletion is working properly
            auto iter = m_modelsMap.begin();
            while (iter != m_modelsMap.end())
            {
                auto modelPtr = iter->first;
                if (currentModels.find(modelPtr) == currentModels.end())
                {   // model was not found in the current frame - ask the FP to remove it
                    AZStd::string errorMessage = "--- Mesh (Run Time) Removal [" + modelPtr->GetModelName() + "]";
                    AZ_Warning("AtomSceneStream", false, errorMessage.c_str());

                    Render::MeshFeatureProcessorInterface::MeshHandle& modelHandle = iter->second;
                    [[maybe_unused]] bool meshReleased = m_meshFeatureProcessor->ReleaseMesh(modelHandle);
                    iter = m_modelsMap.erase(iter);     // erase from map and advance iterator
                }
                else
                {
                    ++iter;
                }
            }
            */

            m_runtime->update();
        }

        bool AtomSceneStreamFeatureProcessor::RegisterMeshForRender(AtomSceneStream::Mesh* currentMesh, Transform& modelTransform)
        {
            if (!currentMesh)
            {
                AZ_Warning("AtomSceneStream", false, "Mesh passed for registration was NULL");
                return false;
            }

            // model was already registered to the mesh feature processor
            if (m_modelsMap.find(currentMesh) != m_modelsMap.end())
            {
                return true;
            }

            // Adding the model to the mesh feature processor to be rendered this frame.
            const Render::MeshHandleDescriptor meshDescriptor = Render::MeshHandleDescriptor{ currentMesh->GetAtomModel()->GetModelAsset() };
            Render::MeshFeatureProcessorInterface::MeshHandle meshHandle = m_meshFeatureProcessor->AcquireMesh(meshDescriptor, currentMesh->GetAtomMaterial());

            AZStd::string errorMessage = "+++ Mesh Registration [" + currentMesh->GetModelName() + "]";
            AZ_Warning("AtomSceneStream", false, errorMessage.c_str() );

            const AZ::Vector3 nonUniformScale(1.0f, 1.0f, 1.0f);
            m_meshFeatureProcessor->SetTransform(meshHandle, modelTransform, nonUniformScale);

            // Register the model to avoid repeating it next frames.
            m_modelsMap[currentMesh] = AZStd::move(meshHandle);
            return true;
        }


        void AtomSceneStreamFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (m_readyForStreaming && !m_isConnectedAndStreaming)
            {
                m_isConnectedAndStreaming = StartUmbraClient();
            }
            else
            {
                UpdateStreamingResources();
            }

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
            if (m_memoryUsage > GPU_MEMORY_LIMIT)
            {
                m_quality *= 0.875f;
                assetLoad.finish(UmbraAssetLoadResult_OutOfMemory);
                return false;
            }

            void* ptr = nullptr;

            switch (assetLoad.getType())
            {
            case UmbraAssetType_Material:
                ptr = new AtomSceneStream::Material(assetLoad);
                break;

            case UmbraAssetType_Texture:
                ptr = new AtomSceneStream::Texture(assetLoad);
                m_memoryUsage += ((AtomSceneStream::Texture*)ptr)->GetMemoryUsage();
                break;

            case UmbraAssetType_Mesh:
                ptr = new AtomSceneStream::Mesh(assetLoad);
                m_memoryUsage += ((AtomSceneStream::Mesh*)ptr)->GetMemoryUsage();
//                AZ::Transform positionTransform = AZ::Transform::CreateIdentity();
//                RegisterMeshForRender((AtomSceneStream::Mesh*)ptr, positionTransform);
                break;

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
            case UmbraAssetType_Material:
            {
                delete (AtomSceneStream::Material*)assetUnload.getUserPointer();
                break;
            }

            case UmbraAssetType_Texture:
            {
                AtomSceneStream::Texture* texture = (AtomSceneStream::Texture*)assetUnload.getUserPointer();
                m_memoryUsage -= texture->GetMemoryUsage();
                delete texture;
                break;
            }

            case UmbraAssetType_Mesh:
            {
                AtomSceneStream::Mesh* meshForRemoval = (AtomSceneStream::Mesh*)assetUnload.getUserPointer();
                m_memoryUsage -= meshForRemoval->GetMemoryUsage();
                if (m_meshFeatureProcessor)
                { 
                    auto iter = m_modelsMap.find(meshForRemoval);
                    if (iter != m_modelsMap.end())
                    {
                        AZStd::string errorMessage = "--- Mesh (Streamer) Removal [" + meshForRemoval->GetModelName() + "]";
                        AZ_Warning("AtomSceneStream", false, errorMessage.c_str());

                        Render::MeshFeatureProcessorInterface::MeshHandle& modelHandle = iter->second;
                        [[maybe_unused]] bool meshReleased = m_meshFeatureProcessor->ReleaseMesh(modelHandle);
                        iter = m_modelsMap.erase(iter);     // erase from map and advance iterator
                    }
                }
                delete meshForRemoval;
                break;
            }

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

        void AtomSceneStreamFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]]RPI::RenderPipelinePtr renderPipeline)
        {
            m_readyForStreaming = true;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
