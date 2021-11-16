#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>
#include <AtomCore/Instance/Instance.h>

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
        class Material
        {
        public:
            // Note that Umbra runtime handles Texture lifetime. Textures have been destroyed before Material is destroyed.
            Material(Umbra::AssetLoad& job);
            ~Material();

        private:
            Data::Instance<AZ::RPI::Material> m_atomMaterial = nullptr;
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
            static AZ::Name s_PositionName;
            static AZ::Name s_NormalName;
            static AZ::Name s_TangentName;
            static AZ::Name s_UVName;
            static AZ::Name s_IndicesName;

            Mesh(Umbra::AssetLoad& job);
            ~Mesh();

        protected:
            bool CreateRenderBuffers();
            AZ::Data::Asset<AZ::RPI::BufferAsset> CreateBufferAsset(
                const void* data,
                const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor,
                const AZStd::string& bufferName);

        private:
            size_t m_usedSize = 0;
            int m_vertexCount = 0;
            int m_indexCount = 0;
            int m_indexBytes = 0;
            Material* m_material = nullptr;   // [Adi] - this can probably go away.
            bool m_isShaded = false;

            // VB streams and IB buffer combined - temporary for GPU buffer creation 
            void* m_buffersData = nullptr;

            // VB and IB Umbra descriptors for the streamer load
            Umbra::ElementBuffer m_vbStreamsDesc[UmbraVertexAttributeCount];
            Umbra::ElementBuffer m_ibDesc;

            AZStd::unordered_map<AZ::Name, AZ::Data::Asset<AZ::RPI::BufferAsset>> m_bufferAssets;
        };

    } // namespace AtomSceneStream
} // namespace AZ
