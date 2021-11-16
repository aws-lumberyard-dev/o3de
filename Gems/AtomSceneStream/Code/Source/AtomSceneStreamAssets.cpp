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
            Data::Asset<AZ::RPI::MaterialAsset> materialAsset = AssetUtils::GetAssetByProductPath<MaterialAsset>(DefaultPbrMaterialPath, AssetUtils::TraceLevel::Assert);
            m_atomMaterial = Material::Create(materialAsset);

            AZ_Error("AtomSceneStream", m_atomMaterial, "Material was not created");

            // And this is an example how to set the textures.
            MaterialPropertyIndex colorProperty = m_atomMaterial->FindPropertyIndex(AZ::Name{ "baseColor.color" });
            m_atomMaterial->SetPropertyValue(colorProperty, color);
        }

        Material::~Material()
        {
            // [Adi] - check if textures have been destroyed at this point.
        }


        //======================================================================
        //                              Mesh
        //======================================================================
        Mesh::s_PositionName = AZ::Name("UmbraMeshPositionBuffer");
        Mesh::s_NormalName = AZ::Name("UmbraMeshNormalBuffer");
        Mesh::s_TangentName = AZ::Name("UmbraMeshTangentBuffer");
        Mesh::s_UVName = AZ::Name("UmbraMeshUVBuffer");
        Mesh::s_IndicesName = AZ::Name("UmbraMeshIndexBuffer");

        //AZ::Data::Instance<AZ::RPI::Model> CreateModelFromProceduralSkinnedMesh(const ProceduralSkinnedMesh& proceduralMesh)


        {
            using namespace AZ;
            Data::AssetId assetId;
            assetId.m_guid = AZ::Uuid::CreateRandom();

            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(Uuid::CreateRandom());

            modelCreator.SetName(AZStd::string("ProceduralSkinnedMesh_" + assetId.m_guid.ToString<AZStd::string>()));

            auto indexBuffer = CreateBufferAsset(proceduralMesh.m_indices.data(), proceduralMesh.m_indices.size(), AZ::RHI::Format::R32_FLOAT);
            auto positionBuffer = CreateBufferAsset(proceduralMesh.m_positions.data(), proceduralMesh.m_positions.size(), AZ::RHI::Format::R32G32B32_FLOAT);
            auto normalBuffer = CreateBufferAsset(proceduralMesh.m_normals.data(), proceduralMesh.m_normals.size(), AZ::RHI::Format::R32G32B32_FLOAT);
            auto tangentBuffer = CreateBufferAsset(proceduralMesh.m_tangents.data(), proceduralMesh.m_tangents.size(), AZ::RHI::Format::R32G32B32A32_FLOAT);
            auto bitangentBuffer = CreateBufferAsset(proceduralMesh.m_bitangents.data(), proceduralMesh.m_bitangents.size(), AZ::RHI::Format::R32G32B32_FLOAT);
            auto uvBuffer = CreateBufferAsset(proceduralMesh.m_uvs.data(), proceduralMesh.m_uvs.size(), AZ::RHI::Format::R32G32_FLOAT);

            //
            // Lod
            //
            RPI::ModelLodAssetCreator modelLodCreator;
            modelLodCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

            modelLodCreator.AddLodStreamBuffer(indexBuffer);
            modelLodCreator.AddLodStreamBuffer(positionBuffer);
            modelLodCreator.AddLodStreamBuffer(normalBuffer);
            modelLodCreator.AddLodStreamBuffer(tangentBuffer);
            modelLodCreator.AddLodStreamBuffer(bitangentBuffer);
            modelLodCreator.AddLodStreamBuffer(uvBuffer);

            //
            // Submesh
            //
            modelLodCreator.BeginMesh();

            // Set the index buffer view
            modelLodCreator.SetMeshIndexBuffer(AZ::RPI::BufferAssetView{ indexBuffer, indexBuffer->GetBufferViewDescriptor() });
            modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), AZ::RPI::BufferAssetView{ positionBuffer, positionBuffer->GetBufferViewDescriptor() });
            modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, AZ::Name(), AZ::RPI::BufferAssetView{ normalBuffer, normalBuffer->GetBufferViewDescriptor() });
            modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, AZ::Name(), AZ::RPI::BufferAssetView{ tangentBuffer, tangentBuffer->GetBufferViewDescriptor() });
            modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "BITANGENT" }, AZ::Name(), AZ::RPI::BufferAssetView{ bitangentBuffer, bitangentBuffer->GetBufferViewDescriptor() });
            modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, AZ::Name(), AZ::RPI::BufferAssetView{ uvBuffer, uvBuffer->GetBufferViewDescriptor() });

            AZ::Aabb localAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1000000.0f, 1000000.0f, 1000000.0f));
            modelLodCreator.SetMeshAabb(AZStd::move(localAabb));

            RPI::ModelMaterialSlot::StableId slotId = 0;
            modelCreator.AddMaterialSlot(RPI::ModelMaterialSlot{ slotId, AZ::Name{}, AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::MaterialAsset>(DefaultSkinnedMeshMaterial) });
            modelLodCreator.SetMeshMaterialSlot(slotId);

            modelLodCreator.EndMesh();

            Data::Asset<RPI::ModelLodAsset> lodAsset;
            modelLodCreator.End(lodAsset);
            modelCreator.AddLodAsset(AZStd::move(lodAsset));

            Data::Asset<RPI::ModelAsset> modelAsset;
            modelCreator.End(modelAsset);

            return RPI::Model::FindOrCreate(modelAsset);
        }


        AZ::Data::Asset<AZ::RPI::BufferAsset> Mesh::CreateBufferAsset(
            const void* data,
            const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor,
            const AZStd::string& bufferName
        )
        {
            AZ::RPI::BufferAssetCreator creator;
            creator.Begin(AZ::Uuid::CreateRandom());

            AZ::RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

            creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
            creator.SetBufferViewDescriptor(bufferViewDescriptor);
            creator.SetUseCommonPool(AZ::RPI::CommonBufferPoolType::StaticInputAssembly);

            AZ::Data::Asset<AZ::RPI::BufferAsset> bufferAsset;
            if (creator.End(bufferAsset))
            {
                return bufferAsset;
            }

            AZ_Error( "AtomSceneStream", false, "Error creating vertex stream %s", bufferName. )
            return AZ::Data::Asset();
        }


        // Reference: TerrainFeatureProcessor::InitializePatchModel()
        bool Mesh::CreateRenderBuffers()
        {
            // Vertex Buffer Streams
            const auto positionDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::vertex), AZ::RHI::Format::R32G32B32_FLOAT);
            const auto positionsAsset = CreateBufferAsset( m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr, positionDesc, "UmbraPositions");

            const auto normalDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::normal), AZ::RHI::Format::R32G32B32_FLOAT);
            const auto normalsAsset = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "UmbraNormals");

            const auto tangentDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::tangent), AZ::RHI::Format::R32G32B32_FLOAT);
            const auto tangentsAsset = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "UmbraTangents");

            const auto uvDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount * sizeof(Vertex::tex), AZ::RHI::Format::R32G32_FLOAT);
            const auto uvsAsset = CreateBufferAsset(patchData.m_uvs.data(), uvBufferViewDesc, "UmbraUVs");

            // Index Buffer
            RHI::Format indicesFormat = (m_indexBytes == 2) ? AZ::RHI::Format::R16_UINT : AZ::RHI::Format::R32_UINT;
            const auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount * m_indexBytes, indicesFormat);
            const auto indicesAsset = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

            return ()
        }

        // Good examples:
        // 1. GltfTrianglePrimitiveBuilder::Create(
        // 2. CreateModelFromProceduralSkinnedMesh
        Mesh::Mesh(Umbra::AssetLoad& job)
        {
            Umbra::MeshInfo info = job.getMeshInfo();

            m_vertexCount = info.numUniqueVertices;
            m_indexCount = info.numIndices;
            m_indexBytes = m_vertexCount < (1 << 16) ? 2 : 4;
            m_usedSize = sizeof(Vertex) * m_vertexCount + m_indexBytes * m_indexCount;
            m_material = (Material*)info.material;

            // Only meshes that have normals can be shaded
            if (info.attributes & (1 << UmbraVertexAttribute_Normal))
                m_isShaded = true;

            // Allocating the permanent data to which the data will be copied - includes both VB streams
            // and IB buffer.
            m_buffersData = malloc(m_usedSize);

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

            CreateRenderBuffers();
        }

        Mesh::~Mesh()
        {
            free(m_buffersData);
            m_buffersData = nullptr;
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ibo);
            g_gpuMemoryUsage -= size;
        }


        Mesh::OrganizedBuffersMemory(Umbra::MeshInfo& info)
        {
            Umbra::MeshInfo info = job.getMeshInfo();

            m_vertexCount = info.numUniqueVertices;
            m_indexCount = info.numIndices;
            m_indexBytes = m_vertexCount < (1 << 16) ? 2 : 4;
            m_material = (Material*)info.material;

            // Only meshes that have normals can be shaded
            if (info.attributes & (1 << UmbraVertexAttribute_Normal))
                m_isShaded = true;

            // Prepare vertex buffer

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertexCount, NULL, GL_STATIC_DRAW);
            Vertex* vertices = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

            // Specify the buffer to which the runtime loads vertex
            Umbra::ElementBuffer vbuffers[UmbraVertexAttributeCount];
            for (int i = 0; i < UmbraVertexAttributeCount; ++i)
            {
                vbuffers[i].flags = UmbraBufferFlags_UncachedMemory;
                vbuffers[i].elementStride = sizeof(Vertex);
                vbuffers[i].elementCapacity = m_vertexCount;
            }
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
