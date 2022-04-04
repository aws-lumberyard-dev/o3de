/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/ValidationLayer.h>

#include "common.h"
#define XR_USE_GRAPHICS_API_VULKAN 1
//
// OpenXR Headers
//
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

namespace AZ
{
    namespace Vulkan
    {
        class FunctionLoader;

        class Instance final
        {
        public:
            AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator, 0);

            struct Descriptor
            {
                RawStringList m_requiredLayers;
                RawStringList m_optionalLayers;
                RawStringList m_requiredExtensions;
                RawStringList m_optionalExtensions;
                AZ::RHI::ValidationMode m_validationMode = RHI::ValidationMode::Disabled;
            };

            static Instance& GetInstance();
            static void Reset();
            ~Instance();
            bool Init(const Descriptor& descriptor);
            void Shutdown();

            VkInstance GetNativeInstance() { return m_instance; }
            const Descriptor& GetDescriptor() const;
            FunctionLoader& GetFunctionLoader() { return *m_functionLoader; }
            StringList GetInstanceLayerNames() const;
            StringList GetInstanceExtensionNames(const char* layerName = nullptr) const;
            RHI::PhysicalDeviceList GetSupportedDevices() const;
            AZ::RHI::ValidationMode GetValidationMode() const;

            void CreateInstanceInternal();
            void LogInstanceInfo();
            std::vector<std::string> GetInstanceExtensions() const;
            void InitializeSystem();
            void LogViewConfigurations();
            void LogEnvironmentBlendMode(XrViewConfigurationType type);
            XrResult GetVulkanGraphicsRequirements2KHR(
                XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkan2KHR* graphicsRequirements);
            XrResult CreateVulkanInstanceKHR(
                XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo, VkInstance* vulkanInstance, VkResult* vulkanResult);

            XrInstance GetXRInstance()
            {
                return m_xrInstance;
            };

            XrSystemId GetXRSystemId()
            {
                return m_systemId;
            };
            XrEnvironmentBlendMode GetEnvironmentBlendMode()
            {
                return m_environmentBlendMode;
            };

            std::vector<const char*> ParseExtensionString(char* names);
            XrViewConfigurationType GetViewConfigType()
            {
                return m_viewConfigType;
            }
        private:
            RHI::PhysicalDeviceList EnumerateSupportedDevices() const;

            Descriptor m_descriptor;
            VkInstance m_instance = VK_NULL_HANDLE;
            AZStd::unique_ptr<FunctionLoader> m_functionLoader;
            RHI::PhysicalDeviceList m_supportedDevices;


            XrInstance m_xrInstance{ XR_NULL_HANDLE };
            XrFormFactor m_formFactor{ XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
            XrViewConfigurationType m_viewConfigType{ XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
            XrEnvironmentBlendMode m_environmentBlendMode{ XR_ENVIRONMENT_BLEND_MODE_OPAQUE };
            XrSystemId m_systemId{ XR_NULL_SYSTEM_ID };
        };
    }
}
