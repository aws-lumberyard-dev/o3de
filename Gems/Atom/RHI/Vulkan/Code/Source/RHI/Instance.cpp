/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <Atom/RHI.Loader/FunctionLoader.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Utils/Utils.h>
#pragma optimize("", off)
namespace AZ
{
    namespace Vulkan
    {
        static const uint32_t s_minVulkanSupportedVersion = VK_API_VERSION_1_0;

        static EnvironmentVariable<Instance> s_vulkanInstance;
        static constexpr const char* s_vulkanInstanceKey = "VulkanInstance";

        Instance& Instance::GetInstance()
        {
            if (!s_vulkanInstance)
            {
                s_vulkanInstance = Environment::FindVariable<Instance>(s_vulkanInstanceKey);
                if (!s_vulkanInstance)
                {
                    s_vulkanInstance = Environment::CreateVariable<Instance>(s_vulkanInstanceKey);
                }
            }

            return s_vulkanInstance.Get();
        }

        void Instance::Reset()
        {
            s_vulkanInstance.Reset();
        }

        Instance::~Instance()
        {
            Shutdown();
        }

        inline std::string GetXrVersionString(XrVersion ver)
        {
            return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
        }

        static void LogLayersAndExtensions()
        {
            // Write out extension properties for a given layer.
            const auto logExtensions = [](const char* layerName, int indent = 0)
            {
                uint32_t instanceExtensionCount;
                CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));

                std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
                for (XrExtensionProperties& extension : extensions)
                {
                    extension.type = XR_TYPE_EXTENSION_PROPERTIES;
                }

                CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(
                    layerName, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data()));

                const std::string indentStr(indent, ' ');
                AZ_Printf("VkLayer", Fmt("%sAvailable Extensions: (%d)\n", indentStr.c_str(), instanceExtensionCount).c_str());
                for (const XrExtensionProperties& extension : extensions)
                {
                    AZ_Printf(
                        "VkLayer",
                        Fmt("%s  Name=%s SpecVersion=%d\n", indentStr.c_str(), extension.extensionName, extension.extensionVersion)
                            .c_str());
                }
            };

            // Log non-layer extensions (layerName==nullptr).
            logExtensions(nullptr);

            // Log layers and any of their extensions.
            {
                uint32_t layerCount;
                CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));

                std::vector<XrApiLayerProperties> layers(layerCount);
                for (XrApiLayerProperties& layer : layers)
                {
                    layer.type = XR_TYPE_API_LAYER_PROPERTIES;
                }

                CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

                AZ_Printf("VkLayer", Fmt("Available Layers: (%d)\n", layerCount).c_str());
                for (const XrApiLayerProperties& layer : layers)
                {
                    AZ_Printf(
                        "VkLayer",
                        Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s\n", layer.layerName,
                            GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description)
                            .c_str());

                    logExtensions(layer.layerName, 4);
                }
            }
        }

        std::vector<std::string> Instance::GetInstanceExtensions() const
        {
            return { XR_KHR_VULKAN_ENABLE_EXTENSION_NAME };
        }
        void Instance::CreateInstanceInternal()
        {
            CHECK(m_xrInstance == XR_NULL_HANDLE);

            // Create union of extensions required by platform and graphics plugins.
            std::vector<const char*> extensions;

            // Transform platform and graphics extension std::strings to C strings.

            const std::vector<std::string> graphicsExtensions = GetInstanceExtensions();
            std::transform(
                graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                [](const std::string& ext)
                {
                    return ext.c_str();
                });

            XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
            createInfo.next = nullptr;
            createInfo.enabledExtensionCount = (uint32_t)extensions.size();
            createInfo.enabledExtensionNames = extensions.data();

            // strncpy(createInfo.applicationInfo.applicationName, "O3de", XR_MAX_APPLICATION_NAME_SIZE);
            azstrncpy(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "O3de", 4);
            createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

            CHECK_XRCMD(xrCreateInstance(&createInfo, &m_xrInstance));
        }

        void Instance::LogInstanceInfo()
        {
            CHECK(m_xrInstance != XR_NULL_HANDLE);

            XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
            CHECK_XRCMD(xrGetInstanceProperties(m_xrInstance, &instanceProperties));

            AZ_Printf(
                "VkLayer",
                Fmt("Instance RuntimeName=%s RuntimeVersion=%s\n", instanceProperties.runtimeName,
                    GetXrVersionString(instanceProperties.runtimeVersion).c_str())
                    .c_str());
        }

        void Instance::LogEnvironmentBlendMode(XrViewConfigurationType type)
        {
            CHECK(m_xrInstance != XR_NULL_HANDLE);
            CHECK(m_systemId != 0);

            uint32_t count;
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemId, type, 0, &count, nullptr));
            CHECK(count > 0);

            AZ_Printf("VkLayer", Fmt("Available Environment Blend Mode count : (%d)\n", count).c_str());

            std::vector<XrEnvironmentBlendMode> blendModes(count);
            CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemId, type, count, &count, blendModes.data()));

            bool blendModeFound = false;
            for (XrEnvironmentBlendMode mode : blendModes)
            {
                const bool blendModeMatch = (mode == m_environmentBlendMode);
                AZ_Printf(
                    "VkLayer", Fmt("Environment Blend Mode (%s) : %s\n", to_string(mode), blendModeMatch ? "(Selected)" : "").c_str());
                blendModeFound |= blendModeMatch;
            }
            CHECK(blendModeFound);
        }

        void Instance::LogViewConfigurations()
        {
            CHECK(m_xrInstance != XR_NULL_HANDLE);
            CHECK(m_systemId != XR_NULL_SYSTEM_ID);

            uint32_t viewConfigTypeCount;
            CHECK_XRCMD(xrEnumerateViewConfigurations(m_xrInstance, m_systemId, 0, &viewConfigTypeCount, nullptr));
            std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
            CHECK_XRCMD(
                xrEnumerateViewConfigurations(m_xrInstance, m_systemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data()));
            CHECK((uint32_t)viewConfigTypes.size() == viewConfigTypeCount);

            AZ_Printf("VkLayer", Fmt("Available View Configuration Types: (%d)\n", viewConfigTypeCount).c_str());
            for (XrViewConfigurationType viewConfigType : viewConfigTypes)
            {
                AZ_Printf(
                    "VkLayer",
                    Fmt("  View Configuration Type: %s %s\n", to_string(viewConfigType),
                        viewConfigType == m_viewConfigType ? "(Selected)" : "")
                        .c_str());

                XrViewConfigurationProperties viewConfigProperties{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
                CHECK_XRCMD(xrGetViewConfigurationProperties(m_xrInstance, m_systemId, viewConfigType, &viewConfigProperties));

                AZ_Printf(
                    "VkLayer",
                    Fmt("  View configuration FovMutable=%s\n", viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False").c_str());

                uint32_t viewCount;
                CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemId, viewConfigType, 0, &viewCount, nullptr));
                if (viewCount > 0)
                {
                    std::vector<XrViewConfigurationView> views(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
                    CHECK_XRCMD(
                        xrEnumerateViewConfigurationViews(m_xrInstance, m_systemId, viewConfigType, viewCount, &viewCount, views.data()));

                    for (uint32_t i = 0; i < views.size(); i++)
                    {
                        const XrViewConfigurationView& view = views[i];

                        AZ_Printf(
                            "VkLayer",
                            Fmt("    View [%d]: Recommended Width=%d Height=%d SampleCount=%d\n", i, view.recommendedImageRectWidth,
                                view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount)
                                .c_str());
                        AZ_Printf(
                            "VkLayer",
                            Fmt("    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d\n", i, view.maxImageRectWidth,
                                view.maxImageRectHeight, view.maxSwapchainSampleCount)
                                .c_str());
                    }
                }
                else
                {
                    AZ_Printf("VkLayer", Fmt("Empty view configuration type\n").c_str());
                }

                LogEnvironmentBlendMode(viewConfigType);
            }
        }

        XrResult Instance::GetVulkanGraphicsRequirements2KHR(
            XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkan2KHR* graphicsRequirements)
        {
            PFN_xrGetVulkanGraphicsRequirementsKHR pfnGetVulkanGraphicsRequirementsKHR = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(
                instance, "xrGetVulkanGraphicsRequirementsKHR",
                reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirementsKHR)));

            XrGraphicsRequirementsVulkanKHR legacyRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
            CHECK_XRCMD(pfnGetVulkanGraphicsRequirementsKHR(instance, systemId, &legacyRequirements));

            graphicsRequirements->maxApiVersionSupported = legacyRequirements.maxApiVersionSupported;
            graphicsRequirements->minApiVersionSupported = legacyRequirements.minApiVersionSupported;

            return XR_SUCCESS;
        }

        void Instance::InitializeSystem()
        {
            CHECK(m_xrInstance != XR_NULL_HANDLE);
            CHECK(m_systemId == XR_NULL_SYSTEM_ID);

            m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; // GetXrFormFactor(m_options->FormFactor);
            m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; // GetXrViewConfigurationType(m_options->ViewConfiguration);
            m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE; // GetXrEnvironmentBlendMode(m_options->EnvironmentBlendMode);

            XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
            systemInfo.formFactor = m_formFactor;
            CHECK_XRCMD(xrGetSystem(m_xrInstance, &systemInfo, &m_systemId));

            AZ_Printf("VkLayer", Fmt("Using system %d for form factor %s\n", m_systemId, to_string(m_formFactor)).c_str());
            CHECK(m_xrInstance != XR_NULL_HANDLE);
            CHECK(m_systemId != XR_NULL_SYSTEM_ID);

            LogViewConfigurations();

            // The graphics API can initialize the graphics device now that the systemId and instance
            // handle are available.
            // m_graphicsPlugin->InitializeDevice(m_instance, m_systemId);

            // Create the Vulkan device for the adapter associated with the system.
            // Extension function must be loaded by name
            XrGraphicsRequirementsVulkan2KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
            CHECK_XRCMD(GetVulkanGraphicsRequirements2KHR(m_xrInstance, m_systemId, &graphicsRequirements));
        }

        // Note: The output must not outlive the input - this modifies the input and returns a collection of views into that modified
        // input!
        std::vector<const char*> Instance::ParseExtensionString(char* names)
        {
            std::vector<const char*> list;
            while (*names != 0)
            {
                list.push_back(names);
                while (*(++names) != 0)
                {
                    if (*names == ' ')
                    {
                        *names++ = '\0';
                        break;
                    }
                }
            }
            return list;
        }

        XrResult Instance::CreateVulkanInstanceKHR(
            XrInstance instance, const XrVulkanInstanceCreateInfoKHR* createInfo,
            VkInstance* vulkanInstance, VkResult* vulkanResult) 
        {
            
            PFN_xrGetVulkanInstanceExtensionsKHR pfnGetVulkanInstanceExtensionsKHR = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(
                instance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanInstanceExtensionsKHR)));

            uint32_t extensionNamesSize = 0;
            CHECK_XRCMD(pfnGetVulkanInstanceExtensionsKHR(instance, createInfo->systemId, 0, &extensionNamesSize, nullptr));

            std::vector<char> extensionNames(extensionNamesSize);
            CHECK_XRCMD(pfnGetVulkanInstanceExtensionsKHR(
                instance, createInfo->systemId, extensionNamesSize, &extensionNamesSize, &extensionNames[0]));
            {
                // Note: This cannot outlive the extensionNames above, since it's just a collection of views into that string!
                std::vector<const char*> extensions = ParseExtensionString(&extensionNames[0]);

                // Merge the runtime's request with the applications requests
                for (uint32_t i = 0; i < createInfo->vulkanCreateInfo->enabledExtensionCount; ++i)
                {
                    extensions.push_back(createInfo->vulkanCreateInfo->ppEnabledExtensionNames[i]);
                }

                VkInstanceCreateInfo instInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
                memcpy(&instInfo, createInfo->vulkanCreateInfo, sizeof(instInfo));
                instInfo.enabledExtensionCount = (uint32_t)extensions.size();
                instInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

                auto pfnCreateInstance = (PFN_vkCreateInstance)createInfo->pfnGetInstanceProcAddr(nullptr, "vkCreateInstance");
                *vulkanResult = pfnCreateInstance(&instInfo, createInfo->vulkanAllocator, vulkanInstance);
            }

            return XR_SUCCESS;
            
            /*
            PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
            CHECK_XRCMD(xrGetInstanceProcAddr(
                instance, "xrCreateVulkanInstanceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnCreateVulkanInstanceKHR)));

            return pfnCreateVulkanInstanceKHR(instance, createInfo, vulkanInstance, vulkanResult);
            */
        }

        bool Instance::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            if (GetValidationMode() != RHI::ValidationMode::Disabled)
            {
                char exeDirectory[AZ_MAX_PATH_LEN];
                AZ::Utils::GetExecutableDirectory(exeDirectory, AZ_ARRAY_SIZE(exeDirectory));

                //This env var (VK_LAYER_PATH) is used by the drivers to look for VkLayer_khronos_validation.dll
                AZ::Utils::SetEnv("VK_LAYER_PATH", exeDirectory, 1);

                RawStringList validationLayers = Debug::GetValidationLayers();
                m_descriptor.m_optionalLayers.insert(m_descriptor.m_requiredLayers.end(), validationLayers.begin(), validationLayers.end());
                m_descriptor.m_optionalExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
                m_descriptor.m_optionalExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
#if defined(AZ_VULKAN_USE_DEBUG_LABELS)
            m_descriptor.m_optionalExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            
            m_functionLoader = FunctionLoader::Create();
            if (!m_functionLoader->Init())
            {
                AZ_Warning("Vulkan", false, "Could not initialized function loader.");
                return false;
            }

            uint32_t apiVersion = VK_API_VERSION_1_0;
            // vkEnumerateInstanceVersion is a Vulkan 1.1 function 
            // so if it's not available we assume Vulkan 1.0
            if (vkEnumerateInstanceVersion && vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, "Failed to get instance version.");
                return false;
            }

            if (apiVersion < s_minVulkanSupportedVersion)
            {
                AZ_Warning(
                    "Vulkan", 
                    false, 
                    "The current Vulkan version (%d.%d.%d) is lower that the minimun required (%d.%d.%d).",
                    VK_VERSION_MAJOR(apiVersion),
                    VK_VERSION_MINOR(apiVersion),
                    VK_VERSION_PATCH(apiVersion),
                    VK_VERSION_MAJOR(s_minVulkanSupportedVersion),
                    VK_VERSION_MINOR(s_minVulkanSupportedVersion),
                    VK_VERSION_PATCH(s_minVulkanSupportedVersion));

                return false;
            }                


            LogLayersAndExtensions();
            CreateInstanceInternal();
            LogInstanceInfo();
            InitializeSystem();

            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.apiVersion = apiVersion;

            VkInstanceCreateInfo instanceCreateInfo = {};
            instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceCreateInfo.pApplicationInfo = &appInfo;

            StringList instanceLayerNames = GetInstanceLayerNames();
            RawStringList optionalLayers = FilterList(m_descriptor.m_optionalLayers, instanceLayerNames);
            m_descriptor.m_requiredLayers.insert(m_descriptor.m_requiredLayers.end(), optionalLayers.begin(), optionalLayers.end());

            StringList instanceExtensions = GetInstanceExtensionNames();
            RawStringList optionalExtensions = FilterList(m_descriptor.m_optionalExtensions, instanceExtensions);
            m_descriptor.m_requiredExtensions.insert(m_descriptor.m_requiredExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());

            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_descriptor.m_requiredLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = m_descriptor.m_requiredLayers.data();
            instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_descriptor.m_requiredExtensions.size());
            instanceCreateInfo.ppEnabledExtensionNames = m_descriptor.m_requiredExtensions.data();

            XrVulkanInstanceCreateInfoKHR createInfo{ XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
            createInfo.systemId = m_systemId;
            createInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
            createInfo.vulkanCreateInfo = &instanceCreateInfo;
            createInfo.vulkanAllocator = nullptr;
            VkResult err;
            CHECK_XRCMD(CreateVulkanInstanceKHR(m_xrInstance, &createInfo, &m_instance, &err));
            //CHECK_VKCMD(err);
            /*
            if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
            {
                AZ_Warning("Vulkan", false, "Failed to create Vulkan instance");
                return false;
            }
            */
            // Now that we have created the instance, load the function pointers for it.
            m_functionLoader->LoadProcAddresses(m_instance, VK_NULL_HANDLE, VK_NULL_HANDLE);

            auto validationMode = GetValidationMode();
            if (validationMode != RHI::ValidationMode::Disabled)
            {
                auto messagesTypeMask = Debug::DebugMessageTypeFlag::Error | Debug::DebugMessageTypeFlag::Warning | Debug::DebugMessageTypeFlag::Performance;
                if (validationMode == RHI::ValidationMode::Verbose)
                {
                    messagesTypeMask |= Debug::DebugMessageTypeFlag::Debug | Debug::DebugMessageTypeFlag::Info;
                }

                Debug::InitDebugMessages(m_instance, messagesTypeMask);
            }
            // Check that we have at least one device that meets the requirements.
            m_supportedDevices = EnumerateSupportedDevices();

            AZ_Warning("Vulkan", !m_supportedDevices.empty(), "Could not find any Vulkan supported device");
            return !m_supportedDevices.empty();
        }

        void Instance::Shutdown() 
        {
            if (m_instance != VK_NULL_HANDLE)
            {
                if (GetValidationMode() != RHI::ValidationMode::Disabled)
                {
                    Debug::ShutdownDebugMessages(m_instance);
                }
                m_supportedDevices.clear();

                vkDestroyInstance(m_instance, nullptr);
                m_instance = VK_NULL_HANDLE;
            }

            if (m_functionLoader)
            {
                m_functionLoader->Shutdown();
            }
            m_functionLoader = nullptr;
        }

        const Instance::Descriptor& Instance::GetDescriptor() const
        {
            return m_descriptor;
        }

        StringList Instance::GetInstanceLayerNames() const
        {
            StringList layerNames;
            uint32_t layerPropertyCount = 0;
            VkResult result = vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr);
            if (IsError(result) || layerPropertyCount == 0)
            {
                return layerNames;
            }

            AZStd::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            result = vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data());
            if (IsError(result))
            {
                return layerNames;
            }

            layerNames.reserve(layerNames.size() + layerProperties.size());
            for (uint32_t layerPropertyIndex = 0; layerPropertyIndex < layerPropertyCount; ++layerPropertyIndex)
            {
                layerNames.emplace_back(layerProperties[layerPropertyIndex].layerName);
            }

            return layerNames;
        }

        StringList Instance::GetInstanceExtensionNames(const char* layerName /*= nullptr*/) const
        {
            StringList extensionNames;
            uint32_t extPropertyCount = 0;
            VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extPropertyCount, nullptr);
            if (IsError(result) || extPropertyCount == 0)
            {
                return extensionNames;
            }

            AZStd::vector<VkExtensionProperties> extProperties;
            extProperties.resize(extPropertyCount);

            result = vkEnumerateInstanceExtensionProperties(layerName, &extPropertyCount, extProperties.data());
            if (IsError(result))
            {
                return extensionNames;
            }

            extensionNames.reserve(extensionNames.size() + extProperties.size());
            for (uint32_t extPropertyIndex = 0; extPropertyIndex < extPropertyCount; extPropertyIndex++)
            {
                extensionNames.emplace_back(extProperties[extPropertyIndex].extensionName);
            }

            return extensionNames;
        }

        RHI::PhysicalDeviceList Instance::GetSupportedDevices() const
        {
            return m_supportedDevices;
        }

        AZ::RHI::ValidationMode Instance::GetValidationMode() const
        {
            return m_descriptor.m_validationMode;
        }

        RHI::PhysicalDeviceList Instance::EnumerateSupportedDevices() const
        {
            RHI::PhysicalDeviceList supportedDevices = PhysicalDevice::Enumerate();
            for (auto it = supportedDevices.begin(); it != supportedDevices.end();)
            {
                PhysicalDevice* physicalDevice = static_cast<PhysicalDevice*>((*it).get());
                const VkPhysicalDeviceProperties& properties = physicalDevice->GetPhysicalDeviceProperties();
                bool shouldIgnore = false;
                // Check that the device supports the minimum required Vulkan version.
                if (properties.apiVersion < s_minVulkanSupportedVersion)
                {
                    AZ_Warning("Vulkan", false, "Ignoring device %s because the Vulkan version doesn't meet the minimum requirements.", properties.deviceName);
                    shouldIgnore = true;
                }

                if (!shouldIgnore)
                {
                    // Check that it supports all required layers.
                    auto layersName = physicalDevice->GetDeviceLayerNames();
                    for (const char* layerName : Device::GetRequiredLayers())
                    {
                        auto findIt = AZStd::find(layersName.begin(), layersName.end(), layerName);
                        if (findIt == layersName.end())
                        {
                            AZ_Warning("Vulkan", false, "Ignoring device %s because required layer %s is not available.", properties.deviceName, layerName);
                            shouldIgnore = true;
                            break;
                        }
                    }

                }

                if (!shouldIgnore)
                {
                    // Check that it supports all required extensions.
                    auto extensionNames = physicalDevice->GetDeviceExtensionNames();
                    for (const char* extensionName : Device::GetRequiredExtensions())
                    {
                        auto findIt = AZStd::find(extensionNames.begin(), extensionNames.end(), extensionName);
                        if (findIt == extensionNames.end())
                        {
                            AZ_Warning("Vulkan", false, "Ignoring device %s because required extension %s is not available.", properties.deviceName, extensionName);
                            shouldIgnore = true;
                            break;
                        }
                    }
                }

                it = shouldIgnore ? supportedDevices.erase(it) : it + 1;
            }
            return supportedDevices;
        }
    }
} // namespace AZ
#pragma optimize("", on)
