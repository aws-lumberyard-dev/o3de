#pragma once

#include <AzCore/base.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>

#include <umbra/Client.hpp>
#include <umbra/Runtime.hpp>

namespace AZ
{
    namespace AtomSceneStream
    {
        #define  CM_TO_METERRS 0.01f

        class Umbra::AssetLoad;

        //======================================================================
        // Texture represents a single texture on GPU
        class Texture
        {
        public:
            // Load texture from AssetLoadJob and create a corresponding OpenGL texture
            Texture(Umbra::AssetLoad& job);
            ~Texture();

            AZStd::string GetName() {  return m_name; }
            uint32_t GetMemoryUsage() { return m_imageDataSize; }
            Data::Instance<RPI::Image> GetStreamingImage() { return m_streamingImage;  }

        private:
            std::vector<uint8_t> m_textureData;

            uint32_t m_imageDataSize = 0;
            Data::Instance<RPI::Image> m_streamingImage;
            AZStd::string m_name;
            static uint32_t s_TextureNumber;
        };

        //======================================================================
        class Material :
            private AZ::Data::AssetBus::MultiHandler
        {
        public:
            // Note that Umbra runtime handles Texture lifetime. Textures have been destroyed before Material is destroyed.
            Material(Umbra::AssetLoad& job);
            ~Material();

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            Data::Instance<RPI::Material> GetAtomMaterial()  { return m_atomMaterial; }

            AZStd::string GetName() { return m_name; }


        private:
            static uint32_t s_MaterialNumber;
            static bool s_useTextures;

            Data::Instance<RPI::Material> m_atomMaterial = nullptr;
            Texture* m_diffuse = nullptr;
            Texture* m_normal = nullptr;
            Texture* m_specular = nullptr;
            bool m_isTransparent = false;

            AZStd::string m_name;
        };

        //======================================================================
        struct Vertex
        {
            float vertex[3];
            float normal[3];
            float tex[2];
            float tangent[3];
        };

        class Mesh
        {
        public:
            Name PositionName;
            Name NormalName;
            Name TangentName;
            Name BiTangentName;
            Name UVName;
            Name IndicesName;

            static uint32_t s_modelNumber;

            Mesh(Umbra::AssetLoad& job);
            ~Mesh();

            Data::Instance<RPI::Model> GetAtomModel() { return m_atomModel; }

            Data::Instance<RPI::Material>  GetAtomMaterial()
            {
                return (m_material ? m_material->GetAtomMaterial() : nullptr);
            }

            uint32_t GetMemoryUsage() { return m_allocatedSize; }
            AZStd::string& GetName() { return m_name;  }
            const Aabb& GetAABB() { return m_aabb;  }
            bool IsReady() { return m_modelReady;  }
            void* GetBuffersData() { return m_buffersData;  }
            uint32_t GetVertexCount() { return m_vertexCount;  }
            uint32_t GetIndexCount() { return m_indexCount; }
            Umbra::ElementBuffer GetVertexDesc() { return m_vbStreamsDesc[UmbraVertexAttribute_Position]; }
            Umbra::ElementBuffer GetIndexDesc() { return m_ibDesc; }

        protected:
            Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const void* data,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                const AZStd::string& bufferName);

            void CalculateTangentsAndBiTangents();
            bool LoadUmbraModel(Umbra::AssetLoad& job);
            bool CreateAtomModel();

        private:
            Aabb m_aabb;
            uint32_t m_allocatedSize = 0;
            uint32_t m_vertexCount = 0;
            uint32_t m_indexCount = 0;
            uint32_t m_indexBytes = 0;
            Material* m_material = nullptr;
            bool m_isShaded = false;

            // VB streams and IB buffer combined - temporary for GPU buffer creation 
            void* m_buffersData = nullptr;
            Data::Instance<RPI::Model> m_atomModel;
            AZStd::string m_name;
            bool m_modelReady = false;

            // VB and IB Umbra descriptors for the streamer load
            Umbra::ElementBuffer m_vbStreamsDesc[UmbraVertexAttributeCount];
            Umbra::ElementBuffer m_ibDesc;

            AZStd::unordered_map<Name, Data::Asset<RPI::BufferAsset>> m_bufferAssets;
        };

    } // namespace AtomSceneStream
} // namespace AZ
