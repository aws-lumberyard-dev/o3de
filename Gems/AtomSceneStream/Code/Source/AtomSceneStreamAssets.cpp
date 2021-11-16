#include <umbra_defs.h>

#pragma once

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
            std::vector<uint8_t>& textureData = m_texturesData[m_currentFrame % backBuffersAmount][textureUsage];
            m_imageDataSize = info.dataByteSize;
            textureData.resize(m_imageDataSize);
            Umbra::ByteBuffer buf = {};
            buf.byteSize = m_imageDataSize;
            buf.flags = 0;
            buf.ptr = textureData.data();
            job.getTextureData(buf);    // [Adi] - are we double allocating here?  why Not go directly to the Source and keep the ptr?

            m_streamingImage = StreamingImage::CreateFromCpuData(streamingImagePool,
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
            g_gpuMemoryUsage -= m_imageDataSize;
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


            // [Adi] - at this point we should create Atom Material - DynamicMaterialTestComponentcan be a good example for that.
            static constexpr const char DefaultPbrMaterialPath[] = "materials/defaultpbr.azmaterial";
            Data::Asset<RPI::MaterialAsset> materialAsset = AssetUtils::GetAssetByProductPath<MaterialAsset>(DefaultPbrMaterialPath, AssetUtils::TraceLevel::Assert);
            m_atomMaterial = Material::Create(materialAsset);

            AZ_Error("AtomSceneStream", m_atomMaterial, "Material was not created");

            // And this is an example how to set the textures.
            MaterialPropertyIndex colorProperty = m_atomMaterial->FindPropertyIndex(Name{ "baseColor.color" });
            m_atomMaterial->SetPropertyValue(colorProperty, color);
        }

        Material::~Material()
        {
            // [Adi] - check if textures have been destroyed at this point.
        }


        //======================================================================
        //                              Mesh
        //======================================================================
        Mesh::s_PositionName = Name("UmbraMeshPositionBuffer");
        Mesh::s_NormalName = Name("UmbraMeshNormalBuffer");
        Mesh::s_TangentName = Name("UmbraMeshTangentBuffer");
        Mesh::s_UVName = Name("UmbraMeshUVBuffer");
        Mesh::s_IndicesName = Name("UmbraMeshIndexBuffer");
        Mesh::s_modelNumber = 0;


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
            if (creator.End(bufferAsset))
            {
                return bufferAsset;
            }

            AZ_Error( "AtomSceneStream", false, "Error creating vertex stream %s", bufferName. )
            return Data::Asset();
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
            modelAssetCreator.SetName(Name("AtomSceneStreamModel_" + modelId.m_guid.ToString<AZStd::string>()));

            {
                // Vertex Buffer Streams
                const auto positionDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::vertex), RHI::Format::R32G32B32_FLOAT);
                const auto positionsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr, positionDesc, "UmbraPositions");

                const auto normalDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::normal), RHI::Format::R32G32B32_FLOAT);
                const auto normalsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr, normalDesc, "UmbraNormals");

                const auto tangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::tangent), RHI::Format::R32G32B32_FLOAT);
                const auto tangentsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr, tangentDesc, "UmbraTangents");

                const auto uvDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::tex), RHI::Format::R32G32_FLOAT);
                const auto uvAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_TextureCoordinate].ptr, uvDesc, "UmbraUVs");

                // Index Buffer
                RHI::Format indicesFormat = (m_indexBytes == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
                const auto indexBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount * m_indexBytes, indicesFormat);
                const auto indicesAsset = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

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
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, s_PositionName, RPI::BufferAssetView{ positionsAsset, positionDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, s_NormalName, RPI::BufferAssetView{ normalsAsset, normalDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, s_TangentName, RPI::BufferAssetView{ tangentsAsset, tangentDesc });
                    modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, s_UVName, RPI::BufferAssetView{ uvAsset, uvDesc });
                    modelLodAssetCreator.SetMeshIndexBuffer({ indicesAsset, indexBufferViewDesc });

                    Aabb localAabb = Aabb::CreateCenterHalfExtents(Vector3(0.0f, 0.0f, 0.0f), Vector3(999999.0f, 999999.0f, 999999.0f));
                    modelLodCreator.SetMeshAabb(AZStd::move(localAabb));

                    modelLodAssetCreator.SetMeshName(Name("AtomSceneStreamModel_%5d", s_modelNumber));
                }

                // Here add the material:
                /*
                RPI::ModelMaterialSlot::StableId slotId = 0;
                modelCreator.AddMaterialSlot(RPI::ModelMaterialSlot{ slotId, Name{}, RPI::AssetUtils::LoadAssetByProductPath<RPI::MaterialAsset>(DefaultSkinnedMeshMaterial) });
                modelLodCreator.SetMeshMaterialSlot(slotId);
                */

                modelLodAssetCreator.EndMesh();
                //----------------------------------------------

                // Next create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                modelLodAssetCreator.End(modelLodAsset);

                // Now add the LOD model asset created to the model asset.
                modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));

                return true;
            }

            // Final stage - create the model based on the created assets
            Data::Asset<RPI::ModelAsset> modelAsset;
            bool success = modelAssetCreator.End(modelAsset);

            m_atomModel = RPI::Model::FindOrCreate(modelAsset);
        }


        // Good examples:
        // 1. GltfTrianglePrimitiveBuilder::Create(
        // 2. CreateModelFromProceduralSkinnedMesh
        Mesh::Mesh(Umbra::MeshInfo& info)
        {
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
                m_vbStreamsDesc[i].elementCapacity = vertexCount;
                m_vbStreamsDesc[i].ptr = (uint8_t*)(m_buffersData + offset);
                m_vbStreamsDesc[i].elementStride = m_streamsElementSize[i];
                m_vbStreamsDesc[i].elementByteSize = m_streamsElementSize[i];

                if (!(info.attributes & (1 << i)))
                {
                    m_vbStreamsDesc[i].ptr = nullptr;
                }
                else
                {   // overall unused can be removed at the end but this is only temporary storage anyway.
                    offset += vertexCount * m_streamsElementSize[i];
                }
            }

            // Prepare index buffer
            m_ibDesc.flags = UmbraBufferFlags_UncachedMemory;
            m_ibDesc.elementCapacity = m_indexCount;
            m_ibDesc.ptr = (uint8_t*)(m_buffersData + offset);
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

            // [Adi] - should we free the memory now that the asets has been created?
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
