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

            if (!m_streamingImage)
            {
                m_imageDataSize = 0;
                AZ_Error("AtomSceneStream", false, "StreamingImage creation failed");
            }
        }

        Texture::~Texture()
        {
            // No need to delete the StreamingImage - once Texture is deleted the ref count will do the rest
            // in a safe way (according to the rest of the ref count by Atom)
        }

        //======================================================================
        //                             Material
        //======================================================================

        void Material::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> materialAsset)
        {
            m_atomMaterial = RPI::Material::Create(materialAsset);

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
                m_atomMaterial->SetPropertyValue(textureIndex, m_diffuse->m_streamingImage );
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

            RPI::MaterialPropertyIndex doubleSidedIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("general.doubleSided"));
            if (doubleSidedIndex.IsValid())
            {
                const bool doubleSided = true; 
                m_atomMaterial->SetPropertyValue(doubleSidedIndex, doubleSided);
            }
            
            Data::AssetBus::MultiHandler::BusDisconnect(materialAsset.GetId());
        }

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
            Data::Asset<RPI::MaterialAsset> materialAsset =
                RPI::AssetUtils::GetAssetByProductPath<RPI::MaterialAsset>(DefaultPbrMaterialPath, RPI::AssetUtils::TraceLevel::Assert);

            if (!materialAsset->IsReady())
            {
                if (!materialAsset.IsLoading())
                {
                    materialAsset.QueueLoad();
                }
                Data::AssetBus::MultiHandler::BusConnect(materialAsset.GetId());
            }
            else
            {
                OnAssetReady(materialAsset);
            }
        }

        Material::~Material()
        {
            // [Adi] - check if textures have been destroyed at this point.
        }


        //======================================================================
        //                              Mesh
        //======================================================================
        uint32_t Mesh::s_modelNumber = 0;

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

        // Calculating the bi-tangent given the normal and tangents.
        // Since the tangents need to be Vector4 with W representing the sign bit, the entire
        // tangent buffer will move to the end of the overall buffer during this operation to
        // avoid the need to copy back.
        void Mesh::CalculateTangentsAndBiTangents()
        {
//            float* position = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr;
            float* normal = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr;
            float* orgTangent = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr;  // this will become the bi-tangent
            float* newTangent = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr + m_vertexCount * 3;
 
//            for (int vtx = 0; vtx < m_vertexCount; ++vtx, position+=3, normal+=3, orgTangent+=3, newTangent+=4)
            for (int vtx = 0; vtx < m_vertexCount; ++vtx, normal += 3, orgTangent += 3, newTangent += 4)
            {
//                Vector3* positionV3 = (Vector3*)position;
                Vector3* normalV3 = (Vector3*)normal;
                Vector3* orgTangentV3 = (Vector3*)orgTangent;

//                *positionV3 *= 0.25f;// CM_TO_METERRS;

//                *((Vector4 *)newTangent) = Vector4(*orgTangentV3);   // moving it to the new buffer location and setting W to 1.0
                memcpy(newTangent, orgTangent, 3 * sizeof(float));
                newTangent[3] = 1.0f;   // Setting the W component of the tangents

                // orgTangent location is now bi-tangent
                Vector3 biTangent = normalV3->Cross(*orgTangentV3);
                memcpy(orgTangent, (void*)&biTangent, 3 * sizeof(float));
//                *orgTangent = normal->Cross(*orgTangent);   // orgTangent now becomes bi-tangent - this will ovverflow last element by 4 bytes
            }
        }

        // References
        //  TerrainFeatureProcessor::InitializePatchModel()
        //  CreateModelFromProceduralSkinnedMesh(const ProceduralSkinnedMesh& proceduralMesh)
        bool Mesh::CreateAtomModel()
        {
            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            m_modelName = "AtomSceneStreamModel_" + AZStd::to_string(s_modelNumber);
            modelAssetCreator.SetName(m_modelName);

            {
                // Vertex Buffer Streams
                const auto positionDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto positionsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr, positionDesc, "UmbraModel_Positions");

                const auto uvDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32_FLOAT);
                const auto uvAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_TextureCoordinate].ptr, uvDesc, "UmbraModel_UVs");

                const auto normalDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto normalsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr, normalDesc, "UmbraModel_Normals");

                // Since we needed to add W component to the tangents and calculate the bi-tangents, the original
                // tangents buffer moved to the end and the bi-tangents took its place.
                const auto bitangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto bitangentsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr, bitangentDesc, "UmbraModel_BiTangents");

                // Tangents in Atom have 4 components - the W component represents R / L hand matrix
                const void* tangentsBufferPtr = ((uint8_t*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr + (m_vertexCount * 3 * sizeof(float)));
                const auto tangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32A32_FLOAT);
                const auto tangentsAsset = CreateBufferAsset(tangentsBufferPtr, tangentDesc, "UmbraModel_Tangents");


                // Index Buffer
                RHI::Format indicesFormat = (m_indexBytes == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
                const auto indexBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount, indicesFormat);
                const auto indicesAsset = CreateBufferAsset(m_ibDesc.ptr, indexBufferViewDesc, "UmbraModel_Indices");

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
                    {
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, PositionName, RPI::BufferAssetView{ positionsAsset, positionDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, NormalName, RPI::BufferAssetView{ normalsAsset, normalDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, TangentName, RPI::BufferAssetView{ tangentsAsset, tangentDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "BITANGENT" }, BiTangentName, RPI::BufferAssetView{ bitangentsAsset, tangentDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, UVName, RPI::BufferAssetView{ uvAsset, uvDesc });
                        modelLodAssetCreator.SetMeshIndexBuffer({ indicesAsset, indexBufferViewDesc });

                        Aabb localAabb = Aabb::CreateCenterHalfExtents(Vector3(0.0f, 0.0f, 0.0f), Vector3(999.0f, 999.0f, 999.0f));
                        modelLodAssetCreator.SetMeshAabb(AZStd::move(localAabb));

                        modelLodAssetCreator.SetMeshName(Name{ m_modelName.c_str() }); // _ % 5d", s_modelNumber));
                    }

                    // And finally add the material associated with the streaming model
                    if (m_material && m_material->GetAtomMaterial())
                    {
                        RPI::ModelMaterialSlot::StableId slotId = 0;
                        modelAssetCreator.AddMaterialSlot(RPI::ModelMaterialSlot{ slotId, Name{"AtomSceneStream_Material"}, m_material->GetAtomMaterial()->GetAsset() });
                        modelLodAssetCreator.SetMeshMaterialSlot(slotId);
                    }
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (modelLodAssetCreator.End(modelLodAsset))
                {
                    // Add the LOD model asset created to the model asset.
                    modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
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

            if (!m_atomModel)
            {
                m_modelName = "FaultyLoadedModel";
                AZ_Error("AtomSceneStream", false, "Error - model could not be found or created");
                return false;
            }

            return true;
        }

        bool Mesh::LoadUmbraModel(Umbra::AssetLoad& job)
        {
            Umbra::MeshInfo info = job.getMeshInfo();

            // Permanent data allocation for the data to be copied includes all VB streams and IB buffer.
            m_buffersData = malloc(m_allocatedSize);

            // Prepare vertex buffer streams - this matches Umbra convention but will require Atom
            // change of Tangents from 3 to 4 floats as well as calculating and adding the bi-tangents
            uint32_t streamsElementSize[UmbraVertexAttributeCount];
            streamsElementSize[UmbraVertexAttribute_Position] = sizeof(Vertex::vertex);
            streamsElementSize[UmbraVertexAttribute_Normal] = sizeof(Vertex::normal);
            streamsElementSize[UmbraVertexAttribute_TextureCoordinate] = sizeof(Vertex::tex);
            streamsElementSize[UmbraVertexAttribute_Tangent] = sizeof(Vertex::tangent); // kept for the Umbra copy but requires 4 components in Atom

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
                    // Because tangents requires 4 components in Atom, offset them to the end, to the
                    // location where the bi-tangent will be set in order to copy back properly at a second pass.
                    // Notice that for the tangent, we reserve 4 floats and not 3 as in the Umbra structure.
                    offset += m_vertexCount * (streamsElementSize[i] + ((i == UmbraVertexAttribute_Tangent) ? sizeof(float) : 0));
                }
            }

            // Taking the account the space required for the bi-tangents stream
            offset += m_vertexCount * streamsElementSize[UmbraVertexAttribute_Normal];

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
                AZ_Error("AtomSceneStream", false, "Error - Umbra model load failure");
            }
            return loadOk;
        }


        // Good examples:
        // 1. GltfTrianglePrimitiveBuilder::Create(
        // 2. CreateModelFromProceduralSkinnedMesh
        Mesh::Mesh(Umbra::AssetLoad& job)
        {
            PositionName = Name{ "UmbraMeshPositionBuffer" };
            NormalName = Name{ "UmbraMeshNormalBuffer" };
            TangentName = Name{ "UmbraMeshTangentBuffer" };
            BiTangentName = Name{ "UmbraMeshBiTangentBuffer" };
            UVName = Name{ "UmbraMeshUVBuffer" };
            IndicesName = Name{ "UmbraMeshIndexBuffer" };

            Umbra::MeshInfo info = job.getMeshInfo();

            m_vertexCount = info.numUniqueVertices;
            m_indexCount = info.numIndices;
            m_indexBytes = m_vertexCount < (1 << 16) ? 2 : 4;
            // Next calculate the required Atom allocation size for
            m_allocatedSize = m_vertexCount * (4 * sizeof(float) + sizeof(Vertex)); // Final Vertex Size = Umbra vertex + added bi-tangent (3 floats) and W component of the Tangent
            m_allocatedSize += m_indexCount * m_indexBytes; // and add the allocation for the indices.
            m_material = (Material*)info.material;

            // Only meshes that have normals can be shaded
            if (info.attributes & (1 << UmbraVertexAttribute_Normal))
                m_isShaded = true;

            if (!LoadUmbraModel(job))
            {
                return;
            }

            // To move from Umbra to Atom we must add the W component to the tangents, create 
            // the bi-tangent stream and calculate the bi-tangents..
            CalculateTangentsAndBiTangents();

            // [Adi] - should we free the memory now that the assets has been created?
            if (CreateAtomModel())
            {
                ++s_modelNumber;
            }
        }

        Mesh::~Mesh()
        {
            free(m_buffersData);
            m_buffersData = nullptr;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
