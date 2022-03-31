/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/PhysicalDevice_Windows.h>
#include <AzCore/std/string/conversions.h>

// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <RHI/DX12_Windows.h>

//struct swapchain_surfdata_t
//{
//    ID3D12DepthStencilView* depth_view;
//    ID3D12RenderTargetView* target_view;
//};
//
//struct swapchain_t
//{
//    XrSwapchain handle;
//    int32_t width;
//    int32_t height;
//    std::vector<XrSwapchainImageD3D12KHR> surface_images;
//    std::vector<swapchain_surfdata_t> surface_data;
//};

struct input_state_t
{
    XrActionSet actionSet;
    XrAction poseAction;
    XrAction selectAction;
    XrPath handSubactionPath[2];
    XrSpace handSpace[2];
    XrPosef handPose[2];
    XrBool32 renderHand[2];
    XrBool32 handSelect[2];
};

// Function pointers for some OpenXR extension methods we'll use.
PFN_xrGetD3D12GraphicsRequirementsKHR ext_xrGetD3D12GraphicsRequirementsKHR = nullptr;
PFN_xrCreateDebugUtilsMessengerEXT ext_xrCreateDebugUtilsMessengerEXT = nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT ext_xrDestroyDebugUtilsMessengerEXT = nullptr;


const XrPosef xr_pose_identity = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
XrInstance xr_instance = {};
XrSession xr_session = {};
XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
bool xr_running = false;
XrSpace xr_app_space = {};
XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;
input_state_t xr_input = {};
XrEnvironmentBlendMode xr_blend = {};
XrDebugUtilsMessengerEXT xr_debug = {};

std::vector<XrView> xr_views;
std::vector<XrViewConfigurationView> xr_config_views;
//std::vector<swapchain_t> xr_swapchains;


namespace AZ
{
    namespace DX12
    {
        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            RHI::PhysicalDeviceList physicalDeviceList;

            DX12Ptr<IDXGIFactoryX> dxgiFactory;
            DX12::AssertSuccess(CreateDXGIFactory2(0, IID_GRAPHICS_PPV_ARGS(dxgiFactory.GetAddressOf())));

            DX12Ptr<IDXGIAdapter> dxgiAdapter;
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters(i, dxgiAdapter.ReleaseAndGetAddressOf()); ++i)
            {
                DX12Ptr<IDXGIAdapterX> dxgiAdapterX;
                dxgiAdapter->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiAdapterX.GetAddressOf()));

                PhysicalDevice* physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(dxgiFactory.Get(), dxgiAdapterX.Get());
                physicalDeviceList.emplace_back(physicalDevice);
            }

            return physicalDeviceList;
        }

        void PhysicalDevice::Init(IDXGIFactoryX* factory, IDXGIAdapterX* adapter)
        {
            // OpenXR will fail to initialize if we ask for an extension that OpenXR
            // can't provide! So we need to check our all extensions before
            // initializing OpenXR with them. Note that even if the extension is
            // present, it's still possible you may not be able to use it. For
            // example: the hand tracking extension may be present, but the hand
            // sensor might not be plugged in or turned on. There are often
            // additional checks that should be made before using certain features!
            std::vector<const char*> use_extensions;
            const char* ask_extensions[] = {
                XR_KHR_D3D12_ENABLE_EXTENSION_NAME, // Use Direct3D12 for rendering
                XR_EXT_DEBUG_UTILS_EXTENSION_NAME, // Debug utils for extra info
            };

            // We'll get a list of extensions that OpenXR provides using this
            // enumerate pattern. OpenXR often uses a two-call enumeration pattern
            // where the first call will tell you how much memory to allocate, and
            // the second call will provide you with the actual data!
            uint32_t ext_count = 0;
            xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
            std::vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
            xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

            printf("OpenXR extensions available:\n");
            for (size_t i = 0; i < xr_exts.size(); i++)
            {
                printf("- %s\n", xr_exts[i].extensionName);

                // Check if we're asking for this extensions, and add it to our use
                // list!
                for (int32_t ask = 0; ask < _countof(ask_extensions); ask++)
                {
                    if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0)
                    {
                        use_extensions.push_back(ask_extensions[ask]);
                        break;
                    }
                }
            }

            // Initialize OpenXR with the extensions we've found!
            XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
            createInfo.enabledExtensionCount = (uint32_t)use_extensions.size();
            createInfo.enabledExtensionNames = use_extensions.data();
            createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
            strcpy_s(createInfo.applicationInfo.applicationName, "Test AppName");
            xrCreateInstance(&createInfo, &xr_instance);

            // Check if OpenXR is on this system, if this is null here, the user
            // needs to install an OpenXR runtime and ensure it's active!
            assert(xr_instance);

            // Load extension methods that we'll need for this application! There's a
            // couple ways to do this, and this is a fairly manual one. Chek out this
            // file for another way to do it:
            // https://github.com/maluoi/StereoKit/blob/master/StereoKitC/systems/platform/openxr_extensions.h
            xrGetInstanceProcAddr(xr_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
            xrGetInstanceProcAddr(xr_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
            xrGetInstanceProcAddr(xr_instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D12GraphicsRequirementsKHR));








            m_dxgiFactory = factory;
            m_dxgiAdapter = adapter;

            DXGI_ADAPTER_DESC adapterDesc;
            adapter->GetDesc(&adapterDesc);

            AZStd::string description;
            AZStd::to_string(description, adapterDesc.Description);

            m_descriptor.m_description = AZStd::move(description);
            m_descriptor.m_type = RHI::PhysicalDeviceType::Unknown; /// DXGI can't tell what kind of device this is?!
            m_descriptor.m_vendorId = static_cast<RHI::VendorId>(adapterDesc.VendorId);
            m_descriptor.m_deviceId = adapterDesc.DeviceId;
            // Note: adapterDesc.Revision is not the driver version. It is "the PCI ID of the revision number of the adapter".
            m_descriptor.m_driverVersion = GetGpuDriverVersion(adapterDesc);
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)] = adapterDesc.DedicatedVideoMemory;
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Host)] = adapterDesc.DedicatedSystemMemory;
        }

        uint32_t PhysicalDevice::GetGpuDriverVersion(const DXGI_ADAPTER_DESC& adapterDesc)
        {
            HKEY dxKeyHandle = nullptr;

            LSTATUS returnCode = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"), 0, KEY_READ, &dxKeyHandle);

            if (returnCode != ERROR_SUCCESS)
            {
                return 0;
            }

            DWORD numOfAdapters = 0;

            returnCode = ::RegQueryInfoKey(
                dxKeyHandle, nullptr, nullptr, nullptr, &numOfAdapters, nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

            if (returnCode != ERROR_SUCCESS)
            {
                return 0;
            }

            constexpr uint32_t SubKeyLength = 256;
            wchar_t subKeyName[SubKeyLength];

            uint32_t driverVersion = 0;

            for (uint32_t i = 0; i < numOfAdapters; ++i)
            {
                DWORD subKeyLength = SubKeyLength;

                returnCode = ::RegEnumKeyEx(dxKeyHandle, i, subKeyName, &subKeyLength, nullptr, nullptr, nullptr, nullptr);

                if (returnCode == ERROR_SUCCESS)
                {
                    DWORD dwordSize = sizeof(uint32_t);
                    DWORD qwordSize = sizeof(uint64_t);
                    DWORD vendorIdRaw{};
                    DWORD deviceId{};
                    uint64_t driverVersionRaw{};

                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("VendorId"), RRF_RT_DWORD, nullptr, &vendorIdRaw, &dwordSize);
                    if (returnCode != ERROR_SUCCESS)
                    {
                        continue;
                    }
                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("DeviceId"), RRF_RT_DWORD, nullptr, &deviceId, &dwordSize);
                    if (returnCode != ERROR_SUCCESS)
                    {
                        continue;
                    }

                    if (vendorIdRaw != adapterDesc.VendorId || deviceId != adapterDesc.DeviceId)
                    {
                        continue;
                    }

                    returnCode = ::RegGetValue(dxKeyHandle, subKeyName, TEXT("DriverVersion"), RRF_RT_QWORD, nullptr, &driverVersionRaw, &qwordSize);

                    if (returnCode != ERROR_SUCCESS)
                    {
                        return 0;
                    }

                    RHI::VendorId vendorId = static_cast<RHI::VendorId>(vendorIdRaw);
                    // Full version number result in the following format:
                    // xx.xx.1x.xxxx (in decimal, each part takes 2 bytes in a QWORD/8 bytes)
                    // [operating system].[DX version].[Driver base line].[Build number]
                    // For the driver base line, different vendors have different format.
                    // For example, Nvidia uses 1x, but Intel uses 1xx.
                    // To align with Vulkan, we take the last 5 digits (in decimal) as the version number, as vendors usually do.
                    uint32_t baseline = (driverVersionRaw & 0xFFFF0000u) >> 16;
                    uint32_t buildNum = driverVersionRaw & 0x0000FFFFu;
                    switch (vendorId)
                    {
                    case RHI::VendorId::nVidia:
                        // From nVidia version format xx.xx.1x.xxxx
                        // to nVidia version format xxx.xx
                        // e.g 27.21.14.5687 -> 456.87 -> Vulkan format
                        driverVersion = (((baseline % 10) * 100 + buildNum / 100) << 22) | ((buildNum % 100) << 14);
                        break;
                    case RHI::VendorId::Intel:
                        // From nVidia version format xx.xx.1xx.xxxx
                        // to nVidia version format 1xx.xxxx
                        // e.g 25.20.100.6793 -> 100.6793 -> Vulkan format
                        driverVersion = (baseline << 14) | buildNum;
                        break;
                    default:
                        driverVersion = (baseline << 22) | (buildNum << 12);
                        
                    }
                    break;
                }
            }

            return driverVersion;
        }

        void PhysicalDevice::Shutdown()
        {
            m_dxgiAdapter = nullptr;
            m_dxgiFactory = nullptr;
        }

        IDXGIFactoryX* PhysicalDevice::GetFactory() const
        {
            return m_dxgiFactory.get();
        }

        IDXGIAdapterX* PhysicalDevice::GetAdapter() const
        {
            return m_dxgiAdapter.get();
        }
    }
}
