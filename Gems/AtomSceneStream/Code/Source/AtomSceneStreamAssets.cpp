#pragma once

#include <AzCore/Math/Aabb.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <umbra_defs.h>

#include <AtomSceneStreamAssets.h>

#pragma optimize("", off)

// All of these were taken with X <--> Y to remove Atom tweak on that
// Y = true, X = false      --> 
// Y = false, X = false     --> 
// Y = false, X = true      --> 
// Y = true, X = true       --> 

// All of these were taken with X <--> Y as per Atom ==>
//      All combinations failed and they seem green when supposed to be red, hence RG switch
// Y = true, X = false      --> NO
// Y = false, X = false     --> NO
// Y = false, X = true      --> NO
// Y = true, X = true       --> NO
static bool flipY = true;  
static bool flipX = false;
static float normalScaleFactor = 3.0f;
static bool disableNormalTexture = false;

namespace AZ
{
    namespace AtomSceneStream
    {
        //======================================================================
        //                             Texture
        //======================================================================
        uint32_t Texture::s_TextureNumber = 0;

        static bool printCreationMessages = false;

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
                { 1, RHI::Format::BC5_UNORM, UmbraTextureFormat_BC5, true }
            };

            Umbra::TextureInfo info = job.getTextureInfo();
            RHI::Format imageFormat = RHI::Format::Unknown;
            uint32_t entries = uint32_t(sizeof(imageFormatPairing) / sizeof(ImageFormatPairing));
            uint32_t entryIndex = 0;
            for ( ; entryIndex < entries; entryIndex++)
            {
                if ((imageFormatPairing[entryIndex].IsLinear == uint32_t(info.colorSpace == UmbraColorSpace_Linear)) &&
                    (imageFormatPairing[entryIndex].FromUmbraFormat == info.format))
                {
                    imageFormat = imageFormatPairing[entryIndex].ToRHIFormat;
                    break;
                }
            }

            if (imageFormat == RHI::Format::Unknown)
            {
                AZ_Error("AtomSceneStream", false, "Error -- Read image format [%d] or linear space [%d] mismatch", (uint32_t)info.format, info.colorSpace);
                return;
            }

            // Creating the Atom streaming image
            {
                uint32_t loadLevel = 0;
                uint32_t curLevelOffset = UmbraTextureGetMipmapLevelOffset(&info, loadLevel);
                uint32_t nextLevelOffset = loadLevel < (uint32_t)info.numMipLevels - 1 ?
                    UmbraTextureGetMipmapLevelOffset(&info, loadLevel + 1)
                    : info.dataByteSize;

                m_imageDataSize = nextLevelOffset - curLevelOffset;

                // getting the streaming texture data
                m_imageDataSize = info.dataByteSize;
                m_textureData.resize(m_imageDataSize);

                Umbra::ByteBuffer buf = {};
                buf.byteSize = m_imageDataSize;
                buf.flags = 0;
                buf.ptr = m_textureData.data() + UmbraTextureGetMipmapLevelOffset(&info, loadLevel);

                // Read the stream data
                job.getTextureData(buf);    // [Adi] - are we double allocating here?  why Not go directly to the Source and keep the ptr?

                // Create the Atom streaming image.
                const Data::Instance<RPI::StreamingImagePool>& streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
                m_streamingImage = RPI::StreamingImage::CreateFromCpuData(*streamingImagePool.get(),
                    RHI::ImageDimension::Image2D,
                    RHI::Size(info.width, info.height, 1),
                    imageFormat,
                    m_textureData.data(),
                    m_imageDataSize);
            }

            if (!m_streamingImage)
            {
                m_imageDataSize = 0;
                AZ_Error("AtomSceneStream", false, "Error -- StreamingImage creation failed");
            }

            m_name = "Texture_" + AZStd::to_string(s_TextureNumber++);
            if (printCreationMessages)
                AZ_Warning("AtomSceneStream", false, "   Texture [%s] was created", m_name.c_str());
        }

        Texture::~Texture()
        {
            // No need to delete the StreamingImage - once Texture is deleted the ref count will do the rest
            // in a safe way (according to the rest of the ref count by Atom)
        }

        //======================================================================
        //                             Material
        //======================================================================
        uint32_t Material::s_MaterialNumber = 0;
        bool Material::s_useTextures = true;

        bool Material::PrepareMaterial()
        {
            if (!m_atomMaterial)
            {
                AZ_Error("AtomSceneStream", false, "Error -- Material was not created");
                return false;
            }

//            RPI::Material* m_atomMaterial = (material != m_atomMaterial) ? material.get() : m_atomMaterial.get();

            // Adding the textures
            RPI::MaterialPropertyIndex useDiffTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.useTexture"));
            RPI::MaterialPropertyIndex useNormTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.useTexture"));
            RPI::MaterialPropertyIndex useSpecTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("specularF0.useTexture"));
            if (s_useTextures)
            {
                RPI::MaterialPropertyIndex diffTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.textureMap"));
                if (m_diffuse->GetStreamingImage() && diffTextureIndex.IsValid() && useDiffTextureIndex.IsValid())
                {
                    m_atomMaterial->SetPropertyValue(diffTextureIndex, m_diffuse->GetStreamingImage());
                    m_atomMaterial->SetPropertyValue(useDiffTextureIndex, true);
                }
                else if (!m_diffuse->GetStreamingImage())
                {
                    AZ_Warning("AtomSceneStream", false, "Warning -- Material [%s] Missing Diffuse Texture", m_name.c_str());
                }

                RPI::MaterialPropertyIndex normTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.textureMap"));
                if (m_normal->GetStreamingImage() && normTextureIndex.IsValid() && useNormTextureIndex.IsValid())
                {
                    m_atomMaterial->SetPropertyValue(normTextureIndex, m_normal->GetStreamingImage());
                    m_atomMaterial->SetPropertyValue(useNormTextureIndex, true);

                    if (flipX)
                    {
                        RPI::MaterialPropertyIndex normFlipXIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.flipX"));
                        if (normFlipXIndex.IsValid())
                        {   // This does not seem to be valid for some reason
                            m_atomMaterial->SetPropertyValue(normFlipXIndex, true);
                        }
                    }

                    if (flipY)
                    {
                        RPI::MaterialPropertyIndex normFlipYIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.flipY"));
                        if (normFlipYIndex.IsValid())
                        {   // This does not seem to be valid for some reason
                            m_atomMaterial->SetPropertyValue(normFlipYIndex, true);
                        }
                    }

                    RPI::MaterialPropertyIndex normalFactorIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("normal.factor"));
                    if (normalFactorIndex.IsValid())
                    {
                        m_atomMaterial->SetPropertyValue(normalFactorIndex, normalScaleFactor);
                    }
                }
                else if (!m_normal->GetStreamingImage())
                {
                    AZ_Warning("AtomSceneStream", false, "Warning -- Material [%s] Missing Normal Texture", m_name.c_str());
                }
                if (disableNormalTexture && useNormTextureIndex.IsValid())
                {
                    m_atomMaterial->SetPropertyValue(useNormTextureIndex, false);
                }

                RPI::MaterialPropertyIndex specTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("specularF0.textureMap"));
                if (m_specular->GetStreamingImage() && specTextureIndex.IsValid() && useSpecTextureIndex.IsValid())
                {
                    m_atomMaterial->SetPropertyValue(specTextureIndex, m_specular->GetStreamingImage());
                    m_atomMaterial->SetPropertyValue(useSpecTextureIndex, true);

                    RPI::MaterialPropertyIndex specFlipYIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("specularF0.flipY"));
                    if (specFlipYIndex.IsValid())
                    {   // This does not seem to be valid for some reason
                        m_atomMaterial->SetPropertyValue(specFlipYIndex, true);
                    }
                }
                else if (!m_specular->GetStreamingImage())
                {
                    AZ_Warning("AtomSceneStream", false, "Warning -- Material [%s] Missing Specular Texture", m_name.c_str());
                }
            }
            else
            {
                m_atomMaterial->SetPropertyValue(useDiffTextureIndex, false);
                m_atomMaterial->SetPropertyValue(useNormTextureIndex, false);
                m_atomMaterial->SetPropertyValue(useSpecTextureIndex, false);

                // And setting a dummy color
                RPI::MaterialPropertyIndex colorIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("baseColor.color"));
                if (colorIndex.IsValid())
                {
                    const Color dummyColor = Color(1.0f, 0.5f, 0.5f, 1.0f);
                    m_atomMaterial->SetPropertyValue(colorIndex, dummyColor);
                }
            }

            RPI::MaterialPropertyIndex roughnessFactorIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("roughness.factor"));
            if (roughnessFactorIndex.IsValid())
            {
                m_atomMaterial->SetPropertyValue(roughnessFactorIndex, 0.5f);
                RPI::MaterialPropertyIndex useRoughTextureIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("roughness.useTexture"));
                m_atomMaterial->SetPropertyValue(useRoughTextureIndex, false);
            }

            if (m_isTransparent)
            {
                RPI::MaterialPropertyIndex opacityModeIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("opacity.mode"));
                RPI::MaterialPropertyIndex factorModeIndex = m_atomMaterial->FindPropertyIndex(AZ::Name("opacity.factor"));
                if (opacityModeIndex.IsValid() && factorModeIndex.IsValid())
                {
                    int opaqueMode = 3; // TintedTransparent
                    m_atomMaterial->SetPropertyValue(opacityModeIndex, opaqueMode);
                    m_atomMaterial->SetPropertyValue(factorModeIndex, 0.7f);
                }
            }

            return m_atomMaterial->Compile();
        }

        void Material::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> materialAsset)
        {
            m_atomMaterial = RPI::Material::Create(materialAsset);

            PrepareMaterial();

            if (printCreationMessages)
                AZ_Warning("AtomSceneStream", false, " Material [%s] was created with Textures: [%s]   [%s]   [%s]",
                    m_name.c_str(), m_diffuse->GetName().c_str(), m_specular->GetName().c_str(), m_normal->GetName().c_str());
            
            Data::AssetBus::MultiHandler::BusDisconnect(materialAsset.GetId());
        }

        // For samples look at cesium-main/Gems/Cesium/Code/Source/GltfRasterMaterialBuilder.cpp
        Material::Material(Umbra::AssetLoad& job)
        {
            m_name = "Material_" + AZStd::to_string(s_MaterialNumber++);

            Umbra::MaterialInfo info = job.getMaterialInfo();

            m_diffuse = (Texture*)info.textures[UmbraTextureType_Diffuse];
            m_normal = (Texture*)info.textures[UmbraTextureType_Normal];
            m_specular = (Texture*)info.textures[UmbraTextureType_Specular];
            m_isTransparent = !!info.transparent;

            // Create the base default Pbr material
//            static constexpr const char DefaultPbrMaterialPath[] = "materials/minimalpbr_default.azmaterial";
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

            AZ_Error("AtomSceneStream", creator.End(bufferAsset), "Error -- creating vertex stream %s", bufferName.c_str());

            return bufferAsset;
        }

        // Calculating the bi-tangent given the normal and tangents.
        // Since the tangents need to be Vector4 with W representing the sign bit, the entire
        // tangent buffer will move to the end of the overall buffer during this operation to
        // avoid the need to copy back.
        bool Mesh::ProcessBuffersData()
        {
            float* position = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr;
            float* normal = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr;
            float* orgTangent = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr;  // this will become the bi-tangent
            float* newTangent = (float*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr + m_vertexCount * 3;
 
//            for (int vtx = 0; vtx < m_vertexCount; ++vtx, position+=3, normal+=3, orgTangent+=3, newTangent+=4)
            for (uint32_t vtx = 0; vtx < m_vertexCount; ++vtx, normal += 3, orgTangent += 3, newTangent += 4, position+=3)
            {
                Vector3* positionV3 = (Vector3*)position;
                Vector3* normalV3 = (Vector3*)normal;
                Vector3* orgTangentV3 = (Vector3*)orgTangent;

//                *((Vector4 *)newTangent) = Vector4(*orgTangentV3);   // moving it to the new buffer location and setting W to 1.0
                memcpy(newTangent, orgTangent, 3 * sizeof(float));
                newTangent[3] = 1.0f;   // Setting the W component of the tangents

                // orgTangent location is now bi-tangent
                Vector3 biTangent = normalV3->Cross(*orgTangentV3);
                memcpy(orgTangent, (void*)&biTangent, 3 * sizeof(float));
//                *orgTangent = normal->Cross(*orgTangent);   // orgTangent now becomes bi-tangent - this will ovverflow last element by 4 bytes

                float length = positionV3->GetLength();
                const float maxVertexSize = 999.0f;
                if (length < maxVertexSize)
                {
                    m_aabb.AddPoint(*positionV3);
                }
                else
                {   // This can happen with Umbra that migth skip last vertices for proper packing and not include them
                    // in the index references
                    AZ_Warning("AtomSceneStream", false, "Warning -- vertex [%d:%d] out of bound (%.2f, %.2f, %.2f) in model [%s]",
                        vtx, m_vertexCount, positionV3->GetX(), positionV3->GetY(), positionV3->GetZ(), m_name.c_str());
                }
            }

            AZ_Error("AtomSceneStream", m_aabb.IsValid(), "Error --- Model [%s] AABB is invalid - all [%d] vertices are corrupted",
                m_name.c_str(), m_vertexCount);
            return m_aabb.IsValid() ? true : false;
        }

        bool Mesh::CreateAtomModel()
        {
            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            modelAssetCreator.SetName(m_name);

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

/*
                // Index Buffer
                // [Adi] [To Do] - the conversion to 4 bytes per index is for debug draw purposes only
                if (m_indexBytes == 2)
                {   // In this case we'd copy the index data into 4 bytes per index and move the ptr to point at
                    // the new starting point.
                    uint16_t* oldPtr = (uint16_t*) m_ibDesc.ptr;
                    uint32_t* newPtr = (uint32_t*) ((uint16_t*)m_ibDesc.ptr + m_indexCount);
                    m_ibDesc.ptr = (void*)newPtr;
                    m_ibDesc.elementByteSize = 4;
                    m_ibDesc.elementStride = 4;
                    for (uint32_t ind = 0; ind < m_indexCount; ++ind)
                    {
                        *newPtr++ = (uint32_t) *oldPtr++;
                    }

                    m_indexBytes = 4;
                }
*/

                RHI::Format indicesFormat = (m_indexBytes == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
                const auto indexBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount, indicesFormat);
                const auto indicesAsset = CreateBufferAsset(m_ibDesc.ptr, indexBufferViewDesc, "UmbraModel_Indices");

                if (!positionsAsset || !normalsAsset || !tangentsAsset || !uvAsset || !indicesAsset)
                {
                    AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - buffer assets were not created successfully", m_name.c_str());
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

                        modelLodAssetCreator.SetMeshAabb(AZStd::move(m_aabb));
                        modelLodAssetCreator.SetMeshName(Name{ m_name.c_str() });
                    }
/*
                    // And finally add the material associated with the streaming model
                    if (m_material && m_material->GetAtomMaterial())
                    {
                        Data::Instance<RPI::Material> atomMaterial = m_material->GetAtomMaterial();
                        RPI::ModelMaterialSlot::StableId slotId = 0;
                        modelAssetCreator.AddMaterialSlot(RPI::ModelMaterialSlot{ slotId, Name{"AtomSceneStream_Material"}, atomMaterial->GetAsset() });
                        modelLodAssetCreator.SetMeshMaterialSlot(slotId);
//                        atomMaterial->Compile();
                    }
                    else
                    {
                        AZStd::string messageStr = m_material ? "Error -- Missing Atom Material [" : " Error -- Missing Umbra Material [" + m_name + "]";
                        AZ_Warning("AtomSceneStream", false, messageStr.c_str());
                    }
                    */
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (!modelLodAssetCreator.End(modelLodAsset))
                {
                    AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - ModelLoadAssetCreator.End() failed", m_name.c_str());
                    return false;
                }

                // Add the LOD model asset created to the model asset.
                modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
            }

            // Final stage - create the model based on the created assets
            Data::Asset<RPI::ModelAsset> modelAsset;

            if (!modelAssetCreator.End(modelAsset))
            {
                AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - model asset was not created", m_name.c_str());
                return false;
            }

            m_atomModel = RPI::Model::FindOrCreate(modelAsset);

            if (!m_atomModel)
            {
                AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - model could not be found or created", m_name.c_str());
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
                AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - Umbra model load failure", m_name.c_str());
            }
            return loadOk;
        }

        Mesh::Mesh(Umbra::AssetLoad& job)
        {
            m_name = "Model_" + AZStd::to_string(s_modelNumber++);
            m_aabb.CreateNull();

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
            m_allocatedSize += m_indexCount * m_indexBytes;// ((m_indexBytes == 2) ? (m_indexBytes * 3) : m_indexBytes); // and add the allocation for the indices.  [Adi][To Do]  - notice doubling for easier debugging
            m_material = (Material*)info.material;

            // Only meshes that have normals can be shaded
            if (info.attributes & (1 << UmbraVertexAttribute_Normal))
                m_isShaded = true;

            if (!LoadUmbraModel(job))
            {
                free(m_buffersData);
                m_buffersData = nullptr;
                return;
            }

            // To move from Umbra to Atom we must add the W component to the tangents, create 
            // the bi-tangent stream and calculate the bi-tangents..
            if (!ProcessBuffersData() || !CreateAtomModel())
            {
                free(m_buffersData);
                m_buffersData = nullptr;
                return;
            }

            m_modelReady = true;
            if (printCreationMessages)
                AZ_Warning("AtomSceneStream", false, "Mesh [%s] was created with Material [%s]", m_name.c_str(), m_material->GetName().c_str());
        }

        Mesh::~Mesh()
        {
            free(m_buffersData);
            m_buffersData = nullptr;
        }

        bool Mesh::Compile(RPI::Scene* scene)
        {
            if (!m_requiresCompile)
            {
                return true;
            }

            Data::Instance<RPI::Material> atomMaterial = m_material->GetAtomMaterial();
            if (atomMaterial->CanCompile())
            {
                bool compiledSuccessfully = true;
                if (atomMaterial->NeedsCompile())
                {
                    compiledSuccessfully = atomMaterial->Compile();
                    AZ_Warning("AtomSceneStream", compiledSuccessfully, "Material [%s] for mesh [%s] FAILED compilation",
                        m_material->GetName().c_str(), m_name.c_str());

                    if (compiledSuccessfully && !m_drawPacket.GetRHIDrawPacket())
                    {
                        compiledSuccessfully = m_drawPacket.Update(*scene, false);
                        AZ_Warning("AtomSceneStream", compiledSuccessfully, "Material [%s] for mesh [%s] FAILED to update drawPacket",
                            m_material->GetName().c_str(), m_name.c_str() );      
                    }
                }
                m_requiresCompile = !compiledSuccessfully;
                return compiledSuccessfully;
            }
            else
            {
                AZ_Warning("AtomSceneStream", false, "Material [%s] for mesh [%s] cannot be compiled at this time",
                    m_material->GetName().c_str(), m_name.c_str());
            }
            return false;
        }

    } // namespace AtomSceneStream
} // namespace AZ

#pragma optimize("", on)
