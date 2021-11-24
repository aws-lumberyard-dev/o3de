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

//#pragma warning( disable : 2220 )

//#include <Eigen/Core>
//#include <Eigen/Geometry>

#include <umbra/Client.hpp>
#include <umbra/Runtime.hpp>

namespace AZ
{
    namespace AtomSceneStream
    {
        class Umbra::AssetLoad;

        //======================================================================
        // Texture represents a single texture on GPU
        class Texture
        {
        public:
            // Load texture from AssetLoadJob and create a corresponding OpenGL texture
            Texture(Umbra::AssetLoad& job);
            ~Texture();

            uint32_t m_imageDataSize = 0;
            Data::Instance<RPI::StreamingImage> m_streamingImage;
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

            Data::Instance<RPI::Material> GetAtomMaterial()
            {
                return m_atomMaterial;
            }

        private:
            Data::Instance<RPI::Material> m_atomMaterial = nullptr;
            Texture* m_diffuse = nullptr;
            Texture* m_normal = nullptr;
            Texture* m_specular = nullptr;
            bool m_isTransparent = false;
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

            Data::Instance<RPI::Model> GetAtomModel()
            {
                return m_atomModel;
            }

            Data::Instance<RPI::Material>  GetAtomMaterial()
            {
                return (m_material ? m_material->GetAtomMaterial() : nullptr);
            }


        protected:
            Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const void* data,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                const AZStd::string& bufferName);

            void CalculateTangentsAndBiTangents();
            bool LoadUmbraModel(Umbra::AssetLoad& job);
            bool CreateAtomModel();

        private:
            size_t m_allocatedSize = 0;
            int m_vertexCount = 0;
            int m_indexCount = 0;
            int m_indexBytes = 0;
            Material* m_material = nullptr;
            bool m_isShaded = false;

            // VB streams and IB buffer combined - temporary for GPU buffer creation 
            void* m_buffersData = nullptr;
            Data::Instance<RPI::Model> m_atomModel;

            // VB and IB Umbra descriptors for the streamer load
            Umbra::ElementBuffer m_vbStreamsDesc[UmbraVertexAttributeCount];
            Umbra::ElementBuffer m_ibDesc;

            AZStd::unordered_map<Name, Data::Asset<RPI::BufferAsset>> m_bufferAssets;
        };

    } // namespace AtomSceneStream
} // namespace AZ
