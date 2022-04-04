/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utils.h>
#include <AtomCore/std/containers/lru_cache.h>
#include <RHI/Buffer.h>
#include <RHI/CommandList.h>
#include <RHI/CommandPool.h>
#include <RHI/CommandListAllocator.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Framebuffer.h>
#include <RHI/NullDescriptorManager.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Queue.h>
#include <RHI/ReleaseQueue.h>
#include <RHI/RenderPass.h>
#include <RHI/Sampler.h>
#include <RHI/SemaphoreAllocator.h>
#include <array>

#pragma optimize("", off)
namespace Side
{
    const int LEFT = 0;
    const int RIGHT = 1;
    const int COUNT = 2;
} // namespace Side


struct SwapchainImageContext
{
    SwapchainImageContext(XrStructureType _swapchainImageType)
        : swapchainImageType(_swapchainImageType)
    {
    }

    // A packed array of XrSwapchainImageVulkan2KHR's for xrEnumerateSwapchainImages
    std::vector<XrSwapchainImageVulkan2KHR> swapchainImages;
    //std::vector<RenderTarget> renderTarget;
    VkExtent2D size{};
    //DepthBuffer depthBuffer{};
    //RenderPass rp{};
    //Pipeline pipe{};
    XrStructureType swapchainImageType;

    SwapchainImageContext() = default;

    std::vector<XrSwapchainImageBaseHeader*> Create(
        VkDevice device,
        //MemoryAllocator* memAllocator,
        uint32_t capacity,
        [[maybe_unused]] const XrSwapchainCreateInfo& swapchainCreateInfo
        //const PipelineLayout& layout,
        //const ShaderProgram& sp,
        //const VertexBuffer<Geometry::Vertex>& vb
        )
    {
        m_vkDevice = device;

        //size = { swapchainCreateInfo.width, swapchainCreateInfo.height };
        //VkFormat colorFormat = (VkFormat)swapchainCreateInfo.format;
        //VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        // XXX handle swapchainCreateInfo.sampleCount

        //depthBuffer.Create(m_vkDevice, memAllocator, depthFormat, swapchainCreateInfo);
        //rp.Create(m_vkDevice, colorFormat, depthFormat);
        //pipe.Create(m_vkDevice, size, layout, rp, sp, vb);

        swapchainImages.resize(capacity);
        //renderTarget.resize(capacity);
        std::vector<XrSwapchainImageBaseHeader*> bases(capacity);
        for (uint32_t i = 0; i < capacity; ++i)
        {
            swapchainImages[i] = { swapchainImageType };
            bases[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&swapchainImages[i]);
        }

        return bases;
    }

    uint32_t ImageIndex(const XrSwapchainImageBaseHeader* swapchainImageHeader)
    {
        auto p = reinterpret_cast<const XrSwapchainImageVulkan2KHR*>(swapchainImageHeader);
        return (uint32_t)(p - &swapchainImages[0]);
    }

    void BindRenderTarget([[maybe_unused]] uint32_t index, VkRenderPassBeginInfo* renderPassBeginInfo)
    {
        //if (renderTarget[index].fb == VK_NULL_HANDLE)
        {
            //renderTarget[index].Create(m_vkDevice, swapchainImages[index].image, depthBuffer.depthImage, size, rp);
        }
        //renderPassBeginInfo->renderPass = rp.pass;
        //renderPassBeginInfo->framebuffer = renderTarget[index].fb;
        renderPassBeginInfo->renderArea.offset = { 0, 0 };
        renderPassBeginInfo->renderArea.extent = size;
    }

private:
    VkDevice m_vkDevice{ VK_NULL_HANDLE };
};

namespace AZ
{
    namespace Vulkan
    {
        class BufferPool;
        class ImagePool;
        class GraphicsPipeline;
        class SwapChain;
        class AsyncUploadQueue;

        class Device final
            : public RHI::Device
        {
            using Base = RHI::Device;
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator, 0);
            AZ_RTTI(Device, "C77D578F-841F-41B0-84BB-EE5430FCF8BC", Base);

            static RHI::Ptr<Device> Create();
            ~Device() = default;

            static RawStringList GetRequiredLayers();
            static RawStringList GetRequiredExtensions();

            VkDevice GetNativeDevice() const;

            uint32_t FindMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const;

            VkMemoryRequirements GetImageMemoryRequirements(const RHI::ImageDescriptor& descriptor);
            VkMemoryRequirements GetBufferMemoryRequirements(const RHI::BufferDescriptor& descriptor);

            const VkPhysicalDeviceFeatures& GetEnabledDevicesFeatures() const;

            VkPipelineStageFlags GetSupportedPipelineStageFlags() const;

            // Some capability of image is restricted by a combination of physical device (GPU) and image format.
            // For example, a GPU cannot use a image of BC1_UNORM format for storage image
            // (and a certain kind of GPU might be able to use it).
            // GetImageUsageFromFormat gives capabilities of images of the given format for this device.
            VkImageUsageFlags GetImageUsageFromFormat(RHI::Format format);

            CommandQueueContext& GetCommandQueueContext();
            const CommandQueueContext& GetCommandQueueContext() const;
            SemaphoreAllocator& GetSemaphoreAllocator();

            const AZStd::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;

            AsyncUploadQueue& GetAsyncUploadQueue();

            RHI::Ptr<Buffer> AcquireStagingBuffer(AZStd::size_t byteCount);

            void QueueForRelease(RHI::Ptr<RHI::Object> object);

            RHI::Ptr<RenderPass> AcquireRenderPass(const RenderPass::Descriptor& descriptor);
            RHI::Ptr<Framebuffer> AcquireFramebuffer(const Framebuffer::Descriptor& descriptor);
            RHI::Ptr<DescriptorSetLayout> AcquireDescriptorSetLayout(const DescriptorSetLayout::Descriptor& descriptor);
            RHI::Ptr<Sampler> AcquireSampler(const Sampler::Descriptor& descriptor);
            RHI::Ptr<PipelineLayout> AcquirePipelineLayout(const PipelineLayout::Descriptor& descriptor);

            RHI::Ptr<CommandList> AcquireCommandList(uint32_t familyQueueIndex, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            RHI::Ptr<CommandList> AcquireCommandList(RHI::HardwareQueueClass queueClass, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

            RHI::Ptr<Memory> AllocateMemory(
                uint64_t sizeInBytes,
                const uint32_t memoryTypeMask,
                const VkMemoryPropertyFlags flags,
                const RHI::BufferBindFlags bufferBindFlags = RHI::BufferBindFlags::None);
            
            uint32_t GetCurrentFrameIndex() const;

            NullDescriptorManager& GetNullDescriptorManager();

            VkBuffer CreateBufferResouce(const RHI::BufferDescriptor& descriptor) const;
            void DestroyBufferResource(VkBuffer vkBuffer) const;

            void InitializeSession();
            XrSession GetSession()
            {
                return m_session;
            }
            std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo);
            void PollEvents(bool* exitRenderLoop, bool* requestRestart);
            void HandleSessionStateChangedEvent(
                const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart);
            const XrEventDataBaseHeader* TryReadNextEvent();
            bool IsSessionFocused() const;
            bool IsSessionRunning() const;
            void LogActionSourceName(XrAction action, const std::string& actionName) const;
            void PollActions();
            void SetSwapchain(SwapChain* swapchain)
            {
                m_swapchain = swapchain;
            }
            SwapChain* GetSwapChain()
            {
                return m_swapchain;
            }

            uint32_t GetSwapchainImageIndex0()
            {
                return m_swapchainImageIndex0;
            }
            uint32_t GetSwapchainImageIndex1()
            {
                return m_swapchainImageIndex1;
            }

            std::map<const XrSwapchainImageBaseHeader*, SwapchainImageContext*> GetSwapchainImageContextMap()
            {
                return m_swapchainImageContextMap;
            }
            XrFrameState GetFrameState()
            {
                return m_frameState;
            }
            bool IsXRActivated()
            {
                return m_isXRRenderBeginCalled;
            }
            RHI::ResultCode BeginXRView(uint32_t viewIndex);
            void EndXRView(uint32_t viewIndex);
            void InitializeActions();
        private:
            Device();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Device
            RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;

            void ShutdownInternal() override;
            RHI::ResultCode BeginFrameInternal() override;
            void EndFrameInternal() override;
            void WaitForIdleInternal() override;
            void CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder) override;
            void UpdateCpuTimingStatisticsInternal() const override;
            AZStd::vector<RHI::Format> GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const override;
            AZStd::chrono::microseconds GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const override;
            void FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities) override;
            RHI::ResultCode InitializeLimits() override;
            void PreShutdown() override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor) override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::BufferDescriptor& descriptor) override;
            void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
            //////////////////////////////////////////////////////////////////////////

            void InitFeaturesAndLimits(const PhysicalDevice& physicalDevice);
            void BuildDeviceQueueInfo(const PhysicalDevice& physicalDevice);

            XrResult CreateVulkanDeviceKHR(
                XrInstance instance,
                const XrVulkanDeviceCreateInfoKHR* createInfo,
                VkDevice* vulkanDevice,
                VkResult* vulkanResult,
                VkPhysicalDevice vkPhysicalDevice,
                VkInstance vkInstance);
            const XrBaseInStructure* GetGraphicsBinding() const;
            void LogReferenceSpaces();
            void CreateVisualizedSpaces();

            template<typename ObjectType>
            using ObjectCache = std::pair<RHI::ObjectCache<ObjectType>, AZStd::mutex>;

            template <typename ObjectType, typename... Args>
            RHI::Ptr<ObjectType> AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args);

            //! Get the vulkan buffer usage flags from buffer bind flags.
            //! Flags will be corrected if required features or extensions are not enabled.
            VkBufferUsageFlags GetBufferUsageFlagBitsUnderRestrictions(RHI::BufferBindFlags bindFlags) const;

            VkDevice m_nativeDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceFeatures m_enabledDeviceFeatures{};
            VkPipelineStageFlags m_supportedPipelineStageFlagsMask = std::numeric_limits<VkPipelineStageFlags>::max();

            AZStd::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
            RHI::Ptr<AsyncUploadQueue> m_asyncUploadQueue;
            CommandListAllocator m_commandListAllocator;
            SemaphoreAllocator m_semaphoreAllocator;

            AZStd::unordered_map<RHI::Format, VkImageUsageFlags> m_imageUsageOfFormat;

            RHI::Ptr<BufferPool> m_stagingBufferPool;

            ReleaseQueue m_releaseQueue;
            CommandQueueContext m_commandQueueContext;

            static const uint32_t RenderPassCacheCapacity = 10000;
            static const uint32_t FrameBufferCacheCapacity = 1000;
            static const uint32_t DescriptorLayoutCacheCapacity = 1000;
            static const uint32_t SamplerCacheCapacity = 1000;
            static const uint32_t PipelineLayoutCacheCapacity = 1000;

            ObjectCache<RenderPass> m_renderPassCache;
            ObjectCache<Framebuffer> m_framebufferCache;
            ObjectCache<DescriptorSetLayout> m_descriptorSetLayoutCache;
            ObjectCache<Sampler> m_samplerCache;
            ObjectCache<PipelineLayout> m_pipelineLayoutCache;

            static const uint32_t MemoryRequirementsCacheSize = 100;
            RHI::ThreadLocalContext<AZStd::lru_cache<uint64_t, VkMemoryRequirements>> m_imageMemoryRequirementsCache;
            RHI::ThreadLocalContext<AZStd::lru_cache<uint64_t, VkMemoryRequirements>> m_bufferMemoryRequirementsCache;

            RHI::Ptr<NullDescriptorManager> m_nullDescriptorManager;

            XrSession m_session{ XR_NULL_HANDLE };
            XrSpace m_appSpace{ XR_NULL_HANDLE };
            XrGraphicsBindingVulkan2KHR m_graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };
            std::vector<XrSpace> m_visualizedSpaces;

            std::list<SwapchainImageContext> m_swapchainImageContexts;
            std::map<const XrSwapchainImageBaseHeader*, SwapchainImageContext*> m_swapchainImageContextMap;
            XrFrameState m_frameState{ XR_TYPE_FRAME_STATE };

            std::vector<XrCompositionLayerBaseHeader*> m_xrLayers;
            XrCompositionLayerProjection m_xrLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
            std::vector<XrCompositionLayerProjectionView> m_projectionLayerViews;

            XrSessionState m_sessionState{ XR_SESSION_STATE_UNKNOWN };
            XrEventDataBuffer m_eventDataBuffer;
            bool m_sessionRunning{ false };

            

            struct InputState
            {
                XrActionSet actionSet{ XR_NULL_HANDLE };
                XrAction grabAction{ XR_NULL_HANDLE };
                XrAction poseAction{ XR_NULL_HANDLE };
                XrAction vibrateAction{ XR_NULL_HANDLE };
                XrAction quitAction{ XR_NULL_HANDLE };
                std::array<XrPath, Side::COUNT> handSubactionPath;
                std::array<XrSpace, Side::COUNT> handSpace;
                std::array<float, Side::COUNT> handScale = { { 1.0f, 1.0f } };
                std::array<XrBool32, Side::COUNT> handActive;
            };
            InputState m_input;
            SwapChain* m_swapchain = nullptr;
            uint32_t m_viewCountOutput = 0;
            bool m_isXRFrameBeginCalled = false;
            bool m_isXRRenderBeginCalled = false;

            uint32_t m_swapchainImageIndex0 = 0;
            uint32_t m_swapchainImageIndex1 = 0;
            CommandQueue* m_graphicsQueue = nullptr;

        };

        template<typename ObjectType, typename ...Args>
        inline RHI::Ptr<ObjectType> Device::AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args)
        {
            AZStd::lock_guard<AZStd::mutex> lock(cache.second);
            auto& cacheContainer = cache.first;
            ObjectType* objectRawPtr = cacheContainer.Find(hash);
            if (!objectRawPtr)
            {
                RHI::Ptr<ObjectType> objectPtr = ObjectType::Create();
                if (objectPtr->Init(AZStd::forward<Args>(args)...) == RHI::ResultCode::Success)
                {
                    objectRawPtr = objectPtr.get();
                    cacheContainer.Insert(hash, AZStd::move(objectPtr));
                }
                else
                {
                    AZ_Error("Vulkan", false, "Failed to create a cached object");
                }
            }
            return RHI::Ptr<ObjectType>(objectRawPtr);
        }
    }
} // namespace AZ

#pragma optimize("", on)
