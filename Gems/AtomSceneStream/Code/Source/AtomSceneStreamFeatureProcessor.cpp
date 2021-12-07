#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>


#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

#include <AzFramework/Components/CameraBus.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

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
        static bool printDebugInfo = true;
        static bool printDebugRemoval = true;
        static bool printDebugAdd = false;
        static bool debugDraw = true;
        static bool debugSpheres = false;
        static bool removeModels = false;

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
            // Turns out that this process is NOT thread safe (well, it will evacuate now but the render might
            // still need the data?)
//            if (!m_runtime)
//                return;

            // run the streaming load for 20 msec - ideally this should be done on another thread!
//            HandleAssetsStreaming(0.025f);
        }

        int AtomSceneStreamFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        void AtomSceneStreamFeatureProcessor::UpdateUmbraViewCamera()
        {
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

            cameraMatrixAtomToUmbra = cameraMatrix;
            // Zero the translation
            cameraMatrixAtomToUmbra.SetTranslation(Vector3::CreateZero());
            ////////////////////////
            cameraMatrixAtomToUmbra.InvertFast();
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;
            // /////////////////////

/*
* Org
            // Rotate the camera in local coordinates to bring to Umbra coordinates system 
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f)) * cameraMatrixAtomToUmbra;
            cameraMatrixAtomToUmbra = Matrix4x4::CreateRotationY(AZ::DegToRad(180.0f)) * cameraMatrixAtomToUmbra;
*/
            // Inverse and transform the position to match Umbra camera position treatment
            cameraPos = -cameraPos;
            Vector3 umbraCameraTranslation = cameraMatrixAtomToUmbra * cameraPos;
            cameraMatrixAtomToUmbra.SetTranslation(umbraCameraTranslation);

            // Camera View-Projection
            AZ::Matrix4x4 worldToClip = viewToClipMatrix * cameraMatrixAtomToUmbra;
            UmbraFloat4_4 umbraWorldToClip;

            worldToClip.StoreToColumnMajorFloat16((float*)&umbraWorldToClip.v[0]);    // Atom and Umbra have transposed matrix usage (column vs' row major)

            // The data 
            Umbra::ViewInfo viewInfo;
            viewInfo.cameraWorldToClip = &umbraWorldToClip;
            viewInfo.cameraPosition = &umbraCameraPos;
            viewInfo.cameraDepthRange = UmbraDepthRange_MinusOneToOne;
            viewInfo.cameraMatrixFormat = UmbraMatrixFormat_ColumnMajor;
            viewInfo.quality = m_quality;
            m_view.setCamera(viewInfo);
        }

        void AtomSceneStreamFeatureProcessor::DebugDraw(
            RPI::AuxGeomDrawPtr auxGeom, AtomSceneStream::Mesh* currentMesh,
            Vector3& offset, const Color& debugColor)
        {
            Vector3 center;
            float radius;

            const Aabb curAABB = currentMesh->GetAABB();
            curAABB.GetAsSphere(center, radius);

            if (debugSpheres)
            {
                auxGeom->DrawSphere(center + offset, radius,
                    debugColor, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off,
                    RPI::AuxGeomDraw::DepthWrite::Off, RPI::AuxGeomDraw::FaceCullMode::None, -1);
            }
            else
            {
                auxGeom->DrawAabb(curAABB,
                    debugColor, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off,
                    RPI::AuxGeomDraw::DepthWrite::Off, RPI::AuxGeomDraw::FaceCullMode::None, -1);
            }
        }

        void AtomSceneStreamFeatureProcessor::UpdateStreamingResources()
        {
            if (!m_runtime)
                return;

            // run the streaming load for 20 msec - ideally this should be done on another thread!
            HandleAssetsStreaming(0.03f);

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

            UpdateUmbraViewCamera();

            m_runtime->update();

            RPI::Scene* scene = GetParentScene();
            RPI::AuxGeomDrawPtr auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(scene);

            ModelsMap currentModels;
            uint32_t modelsNum = 0;
            uint32_t modelsRegistered = 0;

            for (;;)
            {
                Umbra::Renderable batch[128];
                int num = m_view.nextRenderables(batch, sizeof(batch) / sizeof(batch[0]));
                if (!num)
                    break;

                for (int i = 0; i < num; i++)
                {
                    AtomSceneStream::Mesh* currentMesh = (AtomSceneStream::Mesh*)batch[i].mesh;
                    if (!currentMesh->IsReady())
                    {
                        AZ_Warning("AtomSceneStream", false, "Model [%s] not ready yet and will be skipped", currentMesh->GetModelName().c_str());
                        continue;
                    }

                    const float* matrixValues = (const float*)&batch[i].transform.v[0].v[0];
                    Matrix3x4 modelMatrix = Matrix3x4::CreateFromColumnMajorFloat16(matrixValues);
                    Transform modelTransform = Transform::CreateFromMatrix3x4(modelMatrix);
                    Vector3 offset = modelMatrix.GetTranslation();
                    Transform positionTransform = AZ::Transform::CreateIdentity();
                    positionTransform.SetTranslation(offset);

                    if (debugDraw)
                    {
                        DebugDraw(auxGeom, currentMesh, offset, Colors::Green);
                    }

                    ++modelsNum;
                    if (RegisterMeshForRender(currentMesh, positionTransform))
                    {   // add the entry to mark that the model exists in the current frame.
//                        currentModels[currentMesh] = Render::MeshFeatureProcessorInterface::MeshHandle();
                        currentModels[currentMesh->GetModelName()] = Render::MeshFeatureProcessorInterface::MeshHandle();
                        ++modelsRegistered;
                    }
                }
            }
          
            // Remove old entries that should not be rendered anymore.
            // [remark] - this should not be required if the stream offload deletion is working properly
            uint32_t modelsRemoved = 0;

            if (removeModels)
            {
                auto iter = m_modelsMap.begin();
                while (iter != m_modelsMap.end())
                {
    //                AtomSceneStream::Mesh* modelPtr = iter->first;
    //                if (currentModels.find(modelPtr) == currentModels.end())
                    AZStd::string modelName = iter->first;
                    if (currentModels.find(modelName) == currentModels.end())
                    {   // model was not found in the current frame - ask the FP to remove it
                        if (printDebugInfo && printDebugRemoval)
                        {
                            AZStd::string errorMessage = "--- Mesh (Run Time) Removal [" + modelName + "]";
    //                        AZStd::string errorMessage = "--- Mesh (Run Time) Removal [" + modelPtr->GetModelName() + "]";
                            AZ_Warning("AtomSceneStream", false, errorMessage.c_str());
                        }

                        ++modelsRemoved;

                        Render::MeshFeatureProcessorInterface::MeshHandle& meshHandle = iter->second;
                        if (meshHandle.IsValid())
                        {
                            [[maybe_unused]] bool meshReleased = m_meshFeatureProcessor->ReleaseMesh(meshHandle);
                        }
                        else
                        {
                            AZ_Error("AtomSceneStream", false, "Error - mesh handle for [%s] is invalid",modelName.c_str());
                        }
                        iter = m_modelsMap.erase(iter);     // erase from map and advance iterator
                    }
                    else
                    {
                        ++iter;
                    }
                }  
            }


            if (printDebugInfo)// && modelsRemoved)
            {
                AZStd::string statString = "\n======================\nModels Stats - Total[" + AZStd::to_string(modelsNum) + "] - New[" + AZStd::to_string(modelsRegistered) + "] - Removed[" + AZStd::to_string(modelsRemoved) + "]\n=====================\n";
                AZ_Warning("AtomSceneStream", false, statString.c_str());
            }
        }

        void AtomSceneStreamFeatureProcessor::RemoveAllActiveModels()
        {
            auto iter = m_modelsMap.begin();
            while (iter != m_modelsMap.end())
            {
                Render::MeshFeatureProcessorInterface::MeshHandle& modelHandle = iter->second;
                [[maybe_unused]] bool meshReleased = m_meshFeatureProcessor->ReleaseMesh(modelHandle);
                ++iter;
            }
            m_modelsMap.clear();
        }

        bool AtomSceneStreamFeatureProcessor::RegisterMeshForRender(AtomSceneStream::Mesh* currentMesh, Transform& modelTransform)
        {
            if (!currentMesh)
            {
                AZ_Warning("AtomSceneStream", false, "Mesh passed for registration was NULL");
                return false;
            }

            // model was already registered to the mesh feature processor
//            if (m_modelsMap.find(currentMesh) != m_modelsMap.end())
            if (m_modelsMap.find(currentMesh->GetModelName()) != m_modelsMap.end())
            {
                if (printDebugInfo && printDebugAdd)
                {
                    AZStd::string errorMessage = "+++ Mesh Exists [" + currentMesh->GetModelName() + "]";
                    AZ_Warning("AtomSceneStream", false, errorMessage.c_str());
                }
                return true;
            }

            // Adding the model to the mesh feature processor to be rendered this frame.
            const Render::MeshHandleDescriptor meshDescriptor = Render::MeshHandleDescriptor{ currentMesh->GetAtomModel()->GetModelAsset() };
            Render::MeshFeatureProcessorInterface::MeshHandle meshHandle = m_meshFeatureProcessor->AcquireMesh(meshDescriptor, currentMesh->GetAtomMaterial());

            AZ_Error("AtomSceneStream", meshHandle.IsValid(), "Error - mesh handle for [%s] is invalid", currentMesh->GetModelName().c_str());

            if (printDebugInfo && printDebugAdd)
            {
                AZStd::string errorMessage = "+++ Mesh Registration [" + currentMesh->GetModelName() + "]";
                AZ_Warning("AtomSceneStream", false, errorMessage.c_str());
            }

            const AZ::Vector3 nonUniformScale(1.0f, 1.0f, 1.0f);
            m_meshFeatureProcessor->SetTransform(meshHandle, modelTransform, nonUniformScale);

            // Register the model to avoid repeating it next frames.
//            m_modelsMap[currentMesh] = AZStd::move(meshHandle);
            m_modelsMap[currentMesh->GetModelName()] = AZStd::move(meshHandle);
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
            {
                Umbra::MeshInfo info = assetLoad.getMeshInfo();
                AtomSceneStream::Material* material = (AtomSceneStream::Material*)info.material;
                if (!material || !material->GetAtomMaterial())
                {
                    AZ_Warning("AtomSceneStream", false, "Mesh creation postponed until material is ready");
                    break;
                }
                ptr = new AtomSceneStream::Mesh(assetLoad);
                m_memoryUsage += ((AtomSceneStream::Mesh*)ptr)->GetMemoryUsage();
                break;
            }

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
                if (m_meshFeatureProcessor && meshForRemoval)
                {
                    m_memoryUsage -= meshForRemoval->GetMemoryUsage();
//                    auto iter = m_modelsMap.find(meshForRemoval);
                    auto iter = m_modelsMap.find(meshForRemoval->GetModelName());
                    if (iter != m_modelsMap.end())
                    {
                        if (printDebugInfo && printDebugRemoval)
                        {
                            AZStd::string errorMessage = "--- Mesh (Streamer) Removal [" + meshForRemoval->GetModelName() + "]";
                            AZ_Warning("AtomSceneStream", false, errorMessage.c_str());
                        }

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

        void AtomSceneStreamFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
        {
//            RemoveAllActiveModels();
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
