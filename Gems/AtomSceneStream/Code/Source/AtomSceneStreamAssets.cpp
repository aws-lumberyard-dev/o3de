#pragma once

#include <AzCore/Math/Aabb.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <umbra_defs.h>

#include <AtomSceneStreamAssets.h>

#pragma optimize("", off)

namespace AZ
{
    namespace AtomSceneStream
    {
        //======================================================================
        //                             Texture
        //======================================================================
        // Load texture from AssetLoadJob and create a corresponding OpenGL texture
        Texture::Texture(Umbra::AssetLoad& job)
        {
            struct ImageFormatPairing
            {
                uint32_t IsLinear;
                RHI::Format ToRHIFormat;
                UmbraTextureFormat FromUmbraFormat;
                bool IsCompressedTexture;   // compressed format might require different treatment?
            };

            static const ImageFormatPairing imageFormatPairing[] = {
                { 1, RHI::Format::R8G8B8A8_UINT, UmbraTextureFormat_RGBA32, false },
                { 0, RHI::Format::R8G8B8A8_UINT, UmbraTextureFormat_RGBA32, false },
                { 1, RHI::Format::R32G32B32A32_FLOAT, UmbraTextureFormat_RGBA_FLOAT32, false },
                { 0, RHI::Format::R32G32B32A32_FLOAT, UmbraTextureFormat_RGBA_FLOAT32, false },
                // The following two formats are not supported and require error message or handling
                //                { 0, UmbraTextureFormat_RGB24, GL_SRGB8, false },
                //                { 1, UmbraTextureFormat_RGB24, GL_RGB8, false },
                { 1, RHI::Format::BC1_UNORM_SRGB, UmbraTextureFormat_BC1, true },
                { 0, RHI::Format::BC1_UNORM_SRGB, UmbraTextureFormat_BC1, true },
                { 1, RHI::Format::BC3_UNORM_SRGB, UmbraTextureFormat_BC3, true },
                { 0, RHI::Format::BC3_UNORM_SRGB, UmbraTextureFormat_BC3, true },
                { 1, RHI::Format::BC5_SNORM, UmbraTextureFormat_BC5, true }
            };

            Umbra::TextureInfo info = job.getTextureInfo();
            RHI::Format imageFormat = RHI::Format::Unknown;
            uint32_t entries = uint32_t(sizeof(imageFormatPairing) / sizeof(ImageFormatPairing));

            for (uint32_t i = 0; i < entries; i++)
            {
                if ((imageFormatPairing[i].IsLinear == uint32_t(info.colorSpace == UmbraColorSpace_Linear)) &&
                    (imageFormatPairing[i].FromUmbraFormat == info.format))
                {
                    imageFormat = imageFormatPairing[i].ToRHIFormat;
                    break;
                }
            }

            if (imageFormat == RHI::Format::Unknown)
            {
                AZ_Error("AtomSceneStream", false, "Read image format [%d] or linear space [%d] mismatch", (uint32_t)info.format, info.colorSpace);
                return;
            }

            // getting the streaming texture data
            static std::vector<uint8_t> textureData;// = m_texturesData[m_currentFrame % backBuffersAmount][textureUsage];
            m_imageDataSize = info.dataByteSize;
            textureData.resize(m_imageDataSize);

            Umbra::ByteBuffer buf = {};
            buf.byteSize = m_imageDataSize;
            buf.flags = 0;
            buf.ptr = textureData.data();
            job.getTextureData(buf);    // [Adi] - are we double allocating here?  why Not go directly to the Source and keep the ptr?

            const Data::Instance<RPI::StreamingImagePool>& streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
            m_streamingImage = RPI::StreamingImage::CreateFromCpuData(*streamingImagePool.get(),
                RHI::ImageDimension::Image2D,
                RHI::Size(info.width, info.height, 1),
                imageFormat,
                textureData.data(),
                m_imageDataSize);

            AZ_Error("AtomSceneStream", m_streamingImage, "StreamingImage creation failed");
        }

        Texture::~Texture()
        {
            // No need to delete the StreamingImage - once Texture is deleted the ref count will do the rest
            // in a safe way (according to the rest of the ref count by Atom)
//            g_gpuMemoryUsage -= m_imageDataSize;
        }

        //======================================================================
        //                             Material
        //======================================================================
        // For samples look at cesium-main/Gems/Cesium/Code/Source/GltfRasterMaterialBuilder.cpp
        Material::Material(Umbra::AssetLoad& job)
        {
            Umbra::MaterialInfo info = job.getMaterialInfo();

            m_diffuse = (Texture*)info.textures[UmbraTextureType_Diffuse];
            m_normal = (Texture*)info.textures[UmbraTextureType_Normal];
            m_specular = (Texture*)info.textures[UmbraTextureType_Specular];
            m_isTransparent = !!info.transparent;

            // Create the base default Pbr material
            static constexpr const char DefaultPbrMaterialPath[] = "materials/defaultpbr.azmaterial";
            //Data::Asset<RPI::MaterialAsset>
            const auto materialAsset = RPI::AssetUtils::GetAssetByProductPath<RPI::MaterialAsset>(DefaultPbrMaterialPath, RPI::AssetUtils::TraceLevel::Assert);
//            m_atomMaterial = RPI::Material::Create(materialAsset);

            if (!m_atomMaterial)
            {
                AZ_Error("AtomSceneStream", false, "Material was not created");
                return;
            }

/*
            // Adding the textures
            RPI::MaterialPropertyIndex textureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.textureMap"));
            RPI::MaterialPropertyIndex useTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
            if (m_diffuse->m_streamingImage && textureIndex.IsValid() && useTextureIndex.IsValid())
            {
                m_atomMaterial->SetPropertyValue(textureIndex, m_diffuse->m_streamingImage);
                m_atomMaterial->SetPropertyValue(useTextureIndex, true);
            }

            textureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.textureMap"));
            useTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.useTexture"));
            if (m_normal->m_streamingImage && textureIndex.IsValid() && useTextureIndex.IsValid())
            {
                m_atomMaterial->SetPropertyValue(textureIndex, m_normal->m_streamingImage);
                m_atomMaterial->SetPropertyValue(useTextureIndex, true);
            }

            textureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("specularF0.textureMap"));
            useTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("specularF0.useTexture"));
            if (m_specular->m_streamingImage && textureIndex.IsValid() && useTextureIndex.IsValid())
            {
                m_atomMaterial->SetPropertyValue(textureIndex, m_specular->m_streamingImage);
                m_atomMaterial->SetPropertyValue(useTextureIndex, true);
            }
*/
            // And setting a dummy color
            RPI::MaterialPropertyIndex colorIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.color"));
            if (colorIndex.IsValid())
            {
                const Color dummyColor = Color(1.0f, .5f, .5f, 1.0f);
                m_atomMaterial->SetPropertyValue(colorIndex, dummyColor);
            }
        }

        Material::~Material()
        {
            // [Adi] - check if textures have been destroyed at this point.
        }


        //======================================================================
        //                              Mesh
        //======================================================================
        Data::Asset<RPI::BufferAsset> Mesh::CreateBufferAsset(
            const void* data,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            const AZStd::string& bufferName
        )
        {
            RPI::BufferAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

            creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
            creator.SetBufferViewDescriptor(bufferViewDescriptor);
            creator.SetUseCommonPool(RPI::CommonBufferPoolType::StaticInputAssembly);

            Data::Asset<RPI::BufferAsset> bufferAsset;

            AZ_Error("AtomSceneStream", creator.End(bufferAsset), "Error creating vertex stream %s", bufferName.c_str());

            return bufferAsset;
        }


        // References
        //  TerrainFeatureProcessor::InitializePatchModel()
        //  CreateModelFromProceduralSkinnedMesh(const ProceduralSkinnedMesh& proceduralMesh)
        bool Mesh::CreateModel()
        {
            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            modelAssetCreator.SetName("AtomSceneStreamModel");// _" + modelId.m_guid.ToString<AZStd::string>()));

            {
                // Vertex Buffer Streams
                const auto positionDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto positionsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr, positionDesc, "UmbraPositions");

                const auto normalDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto normalsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr, normalDesc, "UmbraNormals");

                const auto tangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto tangentsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr, tangentDesc, "UmbraTangents");

                const auto uvDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32_FLOAT);
                const auto uvAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_TextureCoordinate].ptr, uvDesc, "UmbraUVs");

                // Index Buffer
                RHI::Format indicesFormat = (m_indexBytes == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
                const auto indexBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount, indicesFormat);
                const auto indicesAsset = CreateBufferAsset(m_ibDesc.ptr, indexBufferViewDesc, "TerrainPatchIndices");

                if (!positionsAsset || !normalsAsset || !tangentsAsset || !uvAsset || !indicesAsset)
                {
                    AZ_Error("AtomSceneStream", false, "Error - model buffer assets were not created successfully");
                    return false;
                }

                //--------------------------------------------
                // Creating the model LOD asset
                RPI::ModelLodAssetCreator modelLodAssetCreator;
                modelLodAssetCreator.Begin(Uuid::CreateRandom());

                modelLodAssetCreator.BeginMesh();
                {
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, PositionName, RPI::BufferAssetView{ positionsAsset, positionDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, NormalName, RPI::BufferAssetView{ normalsAsset, normalDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, TangentName, RPI::BufferAssetView{ tangentsAsset, tangentDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, UVName, RPI::BufferAssetView{ uvAsset, uvDesc });
                    modelLodAssetCreator.SetMeshIndexBuffer({ indicesAsset, indexBufferViewDesc });

                    Aabb localAabb = Aabb::CreateCenterHalfExtents(Vector3(0.0f, 0.0f, 0.0f), Vector3(999999.0f, 999999.0f, 999999.0f));
                    modelLodAssetCreator.SetMeshAabb(AZStd::move(localAabb));

                    modelLodAssetCreator.SetMeshName(Name{ "AtomSceneStream_Model" }); // _ % 5d", s_modelNumber));
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (modelLodAssetCreator.End(modelLodAsset))
                {
                    // Add the LOD model asset created to the model asset.
                    modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
                }

                // And finally add the material associated with the streaming model
                if (m_material && m_material->GetAtomMaterial())
                {
                    RPI::ModelMaterialSlot::StableId slotId = 0;
                    modelAssetCreator.AddMaterialSlot(RPI::ModelMaterialSlot{ slotId, Name{"AtomSceneStream_Material"}, m_material->GetAtomMaterial()->GetAsset() });
                    modelLodAssetCreator.SetMeshMaterialSlot(slotId);
                }
            }


            // Final stage - create the model based on the created assets
            Data::Asset<RPI::ModelAsset> modelAsset;

            if (!modelAssetCreator.End(modelAsset))
            {
                AZ_Error("AtomSceneStream", false, "Error - model asset was not created");
                return false;
            }

            m_atomModel = RPI::Model::FindOrCreate(modelAsset);
            AZ_Error("AtomSceneStream", m_atomModel, "Error - model could not be found or created");

            return m_atomModel ? true : false;
        }

        uint32_t Mesh::s_modelNumber = 0;

        // Good examples:
        // 1. GltfTrianglePrimitiveBuilder::Create(
        // 2. CreateModelFromProceduralSkinnedMesh
        Mesh::Mesh(Umbra::AssetLoad& job)
        {
            PositionName = Name{ "UmbraMeshPositionBuffer" };
            NormalName = Name{ "UmbraMeshNormalBuffer" };
            TangentName = Name{ "UmbraMeshTangentBuffer" };
            UVName = Name{ "UmbraMeshUVBuffer" };
            IndicesName = Name{ "UmbraMeshIndexBuffer" };

            Umbra::MeshInfo info = job.getMeshInfo();
            ++s_modelNumber;

            m_vertexCount = info.numUniqueVertices;
            m_indexCount = info.numIndices;
            m_indexBytes = m_vertexCount < (1 << 16) ? 2 : 4;
            m_allocatedSize = sizeof(Vertex) * m_vertexCount + m_indexBytes * m_indexCount;
            m_material = (Material*)info.material;

            // Only meshes that have normals can be shaded
            if (info.attributes & (1 << UmbraVertexAttribute_Normal))
                m_isShaded = true;

            // Allocating the permanent data to which the data will be copied - includes both VB streams
            // and IB buffer.
            m_buffersData = malloc(m_allocatedSize);

            // Prepare vertex buffer streams
            uint32_t streamsElementSize[UmbraVertexAttributeCount];
            streamsElementSize[UmbraVertexAttribute_Position] = sizeof(Vertex::vertex);
            streamsElementSize[UmbraVertexAttribute_Normal] = sizeof(Vertex::normal);
            streamsElementSize[UmbraVertexAttribute_TextureCoordinate] = sizeof(Vertex::tex);
            streamsElementSize[UmbraVertexAttribute_Tangent] = sizeof(Vertex::tangent);

            // Specify the buffer to which the runtime loads vertex
            uint32_t offset = 0;
            for (int i = 0; i < UmbraVertexAttributeCount; ++i)
            {
                m_vbStreamsDesc[i].flags = UmbraBufferFlags_UncachedMemory;
                m_vbStreamsDesc[i].elementCapacity = m_vertexCount;
                m_vbStreamsDesc[i].ptr = (uint8_t*)m_buffersData + offset;
                m_vbStreamsDesc[i].elementStride = streamsElementSize[i];
                m_vbStreamsDesc[i].elementByteSize = streamsElementSize[i];

                if (!(info.attributes & (1 << i)))
                {
                    m_vbStreamsDesc[i].ptr = nullptr;
                }
                else
                {   // overall unused can be removed at the end but this is only temporary storage anyway.
                    offset += m_vertexCount * streamsElementSize[i];
                }
            }

            // Prepare index buffer
            m_ibDesc.flags = UmbraBufferFlags_UncachedMemory;
            m_ibDesc.elementCapacity = m_indexCount;
            m_ibDesc.ptr = (uint8_t*)m_buffersData + offset;
            m_ibDesc.elementStride = m_indexBytes;
            m_ibDesc.elementByteSize = m_indexBytes;


            // Read vertex and index buffers directly into memory mapped buffers. This performs mesh decompression, which
            // can take longer than a frame. In order to not see frame drops, there are two solutions:
            //   1) Run mesh decompression in a separate thread
            //   2) Decompress mesh in smaller parts with time budget
            bool loadOk;
#if 1
            loadOk = job.getMeshData(m_vbStreamsDesc, m_ibDesc);
#else // This branch shows that mesh can be loaded in parts
            loadOk = job.setMeshBuffers(m_vbStreamsDesc, m_ibDesc);
            while (!job.meshLoadDone() && loadOk)
            {
                uint32_t vertsLoaded, indicesLoaded;
                loadOk = job.meshLoadNext(&vertsLoaded, &indicesLoaded);
            }
#endif

            if (!loadOk)
            {
                free(m_buffersData);
                return;
            }

            // [Adi] - should we free the memory now that the assets has been created?
            CreateModel();
        }

        Mesh::~Mesh()
        {
            free(m_buffersData);
            m_buffersData = nullptr;
            //            g_gpuMemoryUsage -= m_allocatedSize;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
