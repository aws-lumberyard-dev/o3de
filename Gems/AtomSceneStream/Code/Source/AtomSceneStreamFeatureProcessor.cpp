#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Debug/EventTrace.h>

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

#include <AtomSceneStreamFeatureProcessor.h>

#pragma optimize("", off)

namespace AZ
{
    namespace AtomSceneStream
    {
        uint32_t AtomSceneStreamFeatureProcessor::s_instanceCount = 0;

        AtomSceneStreamFeatureProcessor::AtomSceneStreamFeatureProcessor()
        {
            ++s_instanceCount;

            RestartUmbraCLient();
        }

        AtomSceneStreamFeatureProcessor::RestartUmbraCLient()
        {
            const char* apiKey = "";   // in our case should be empty string as per the mail
            // Replace the following with data from a component
            const char* locator = "v=1&project=atom%40amazon.com&model=874639baa2218cf4f35019d0dca45c1f4a3435e4&version=default&endpoint=https%3A%2F%2Fhorizon-api-stag.hazel.dex.robotics.a2z.com%2F";

            // Create a Client
            Umbra::Client client = Umbra::Client("RuntimeSample");

            // Create Umbra runtime
            Umbra::EnvironmentInfo env;
            UmbraEnvironmentInfoDefaults(&env);
            // Texture support flags is the set of textures that are supported. Runtime attempts to deliver only textures
            // that are supported.
            env.textureSupportFlags = UmbraTextureSupportFlags_BC1 | UmbraTextureSupportFlags_BC3 |
                UmbraTextureSupportFlags_BC5 | UmbraTextureSupportFlags_Float;

            Umbra::Runtime runtime(*client, env);

            // Create a Scene by connecting it to a model
            Umbra::Scene scene = runtime.createScene(apiKey, locator);

            // Wait for connection so that camera can be initialized
            for (;;)
            {
                Umbra::ConnectionStatus s = scene.getConnectionStatus();
                if (s == UmbraConnectionStatus_Connected)
                    break;
                else if (s == UmbraConnectionStatus_ConnectionError)
                {
                    AZ_Error("AtomSceneStream", false, "Error connecting to Umbra Back end");
                    return;
                }
            }

            // Create a View
            Umbra::View view = runtime.createView();
        }

        AtomSceneStreamFeatureProcessor::~AtomSceneStreamFeatureProcessor()
        {
        }

        void AtomSceneStreamFeatureProcessor::Reflect(ReflectContext* context)
        {
            HairGlobalSettings::Reflect(context);

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

            ++m_currentFrame;
            UpdateLoadedAssets();

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
        bool AtomSceneStreamFeatureProcessor::AssetLoad(Umbra::Runtime& runtime)
        {
            Umbra::AssetLoad assetLoad = runtime.getNextAssetLoad();
            if (!assetLoad)
                return false;

            // If memory usage is too high, the job is finished with OutOfMemory. This tells Umbra to stop loading more. The
            // quality must be reduced so that loading can continue
            if (g_gpuMemoryUsage > GPU_MEMORY_LIMIT)
            {
                m_modelsQuality *= 0.875f;
                assetLoad.finish(UmbraAssetLoadResult_OutOfMemory);
                return false;
            }

            void* ptr = nullptr;

            switch (assetLoad.getType())
            {
            case UmbraAssetType_Material: ptr = new Material(assetLoad); break;
            case UmbraAssetType_Texture: ptr = new Texture(assetLoad); break;
            case UmbraAssetType_Mesh: ptr = new Mesh(assetLoad.getMeshInfo()); break;
            default: break;
            }

            assetLoad.prepare((Umbra::UserPointer)ptr);

            // Finish the job
            assetLoad.finish(UmbraAssetLoadResult_Success);
            return true;
        }

        // AssetUnloadJob tells that an asset can be freed from the GPU. This function frees a single asset
        bool AtomSceneStreamFeatureProcessor::AssetUnload(Umbra::Runtime& runtime)
        {
            Umbra::AssetUnload assetUnload = runtime.getNextAssetUnload();
            if (!assetUnload)
                return false;

            switch (assetUnload.getType())
            {
            case UmbraAssetType_Material: delete (Material*)assetUnload.getUserPointer(); break;
            case UmbraAssetType_Texture: delete (Texture*)assetUnload.getUserPointer(); break;
            case UmbraAssetType_Mesh: delete (Mesh*)assetUnload.getUserPointer(); break;
            default: break;
            }

            assetUnload.finish();
            return true;
        }

        // Perform asset streaming work until time budget is reached
        void AtomSceneStreamFeatureProcessor::HandleAssets(Umbra::Runtime& runtime, float seconds)
        {
            const float endTime = (float)glfwGetTime() + seconds;

            // First unload old assets to make room for new ones
            while (glfwGetTime() <= endTime)
                if (!AssetUnload(runtime))
                    break;

            while (glfwGetTime() <= endTime)
                if (!wAssetLoad(runtime))
                    break;
        }

        //! textureUsage = diffuse / specular / ..
        Data::Instance<RPI::StreamingImage> AtomSceneStreamFeatureProcessor::CreateStreamingTexture(Umbra::AssetLoad& job, uint32_t textureUsage)
        {
            struct ImageFormatPairing
            {
                uint32_t IsLinear;
                RHI::Format ToRHIFormat;
                uint32_t FromUmbraFormat;
                uint32_t DataType;
            };

            static const textureFormatConverter imageFormatPairing[] = {
                { 1, RHI::Format::R8G8B8A8_UINT, UmbraTextureFormat_RGBA32, GL_UNSIGNED_BYTE },
                { 0, RHI::Format::R8G8B8A8_UINT, UmbraTextureFormat_RGBA32, GL_UNSIGNED_BYTE },
                { 1, RHI::Format::R32G32B32A32_FLOAT, UmbraTextureFormat_RGBA_FLOAT32, GL_FLOAT },
                { 0, RHI::Format::R32G32B32A32_FLOAT, UmbraTextureFormat_RGBA_FLOAT32, GL_FLOAT },
// The following two formats are not supported and require error message or handling
//                { 0, UmbraTextureFormat_RGB24, GL_SRGB8, GL_UNSIGNED_BYTE },
//                { 1, UmbraTextureFormat_RGB24, GL_RGB8, GL_UNSIGNED_BYTE },
                { 1, RHI::Format::BC1_UNORM_SRGB, UmbraTextureFormat_BC1, 0 },
                { 0, RHI::Format::BC1_UNORM_SRGB, UmbraTextureFormat_BC1, 0 },
                { 1, RHI::Format::BC3_UNORM_SRGB, UmbraTextureFormat_BC3, 0 },
                { 0, RHI::Format::BC3_UNORM_SRGB, UmbraTextureFormat_BC3, 0 },
                { 1, RHI::Format::BC5_SNORM, UmbraTextureFormat_BC5, 0 }
            };

            Umbra::TextureInfo info = job.getTextureInfo();
            RHI::Format imageFormat = RHI::Format::Unknown;

            for (int i = 0; i < int(sizeof(imageFormatPairing) / sizeof(ImageFormatPairing)); i++)
            {
                if (imageFormatPairing[i].IsLinear == (info.colorSpace == UmbraColorSpace_Linear) && imageFormatPairing[i].FromUmbraFormat == info.format)
                {
                    imageFormat = imageFormatPairing[i].ToRHIFormat;
                    break;
                }
            }

            if (imageFormat.ToRHIFormat == RHI::Format::Unknown)
            {
                AZ_Error("AtomSceneStream", false, "Read image format [%d] != [%d] or linear space [%d] != [%d] mismatch",
                    glFormats[i].FromUmbraFormat, info.format,
                    imageFormatPairing[i].IsLinear, info.colorSpace);
                return;
            }

            Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();

            // getting the streaming texture data
            std::vector<uint8_t> &textureData = m_texturesData[m_currentFrame % backBuffersAmount][textureUsage];
            uint32_t imageDataSize = info.dataByteSize;
            textureData.resize(imageDataSize);
            Umbra::ByteBuffer buf = {};
            buf.byteSize = imageDataSize;
            buf.flags = 0;
            buf.ptr = textureData.data();
            job.getTextureData(buf);

            return StreamingImage::CreateFromCpuData( streamingImagePool,
                RHI::ImageDimension::Image2D,
                RHI::Size(info.width, info.height, 1),
                imageFormat,
                textureData.data(),
                imageDataSize);
        }

        // Review LuxCoreMaterial::ParseTexture for texture load / set
        bool AtomSceneStreamFeatureProcessor::CreateStreamingMaterial(Umbra::AssetLoad& job)
        {

            Umbra::MaterialInfo info = job.getMaterialInfo();
            diffuse = (Texture*)info.textures[UmbraTextureType_Diffuse];
            normal = (Texture*)info.textures[UmbraTextureType_Normal];
            specular = (Texture*)info.textures[UmbraTextureType_Specular];
            transparent = !!info.transparent;

            static constexpr const char DefaultPbrMaterialPath[] = "materials/defaultpbr.azmaterial";
            AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset = AssetUtils::GetAssetByProductPath<MaterialAsset>(DefaultPbrMaterialPath, AssetUtils::TraceLevel::Assert);
            AZ::Data::Instance<AZ::RPI::Material> material = Material::Create(materialAsset);

            MaterialPropertyIndex colorProperty = material->FindPropertyIndex(AZ::Name{ "baseColor.color" });
            material->SetPropertyValue(colorProperty, color);
        }

        void AtomSceneStreamFeatureProcessor::CreateMesh(Umbra::AssetLoad& job)
        {
            // Review MiniView::nextRenderables for more understanding how to cache the scene
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
