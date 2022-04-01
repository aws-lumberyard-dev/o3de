/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <RHI/Device.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/NsightAftermath.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/WindowsVersionQuery.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/FactoryManagerBus.h>
#include <comdef.h>


// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

//#include <RHI/DX12_Windows.h>


struct swapchain_surfdata_t
{
    ID3D12DepthStencilView* depth_view;
    ID3D12RenderTargetView* target_view;
};

struct swapchain_t
{
    XrSwapchain handle;
    int32_t width;
    int32_t height;
    std::vector<XrSwapchainImageD3D12KHR> surface_images;
    std::vector<swapchain_surfdata_t> surface_data;
};

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

// struct app_transform_buffer_t
//{
//    XMFLOAT4X4 world;
//    XMFLOAT4X4 viewproj;
//};

XrFormFactor app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

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
std::vector<swapchain_t> xr_swapchains;

int64_t d3d_swapchain_fmt = DXGI_FORMAT_R8G8B8A8_UNORM;


swapchain_surfdata_t d3d_make_surface_data(ID3D12DeviceX* d3d_device, XrBaseInStructure& swapchain_img)
{
    swapchain_surfdata_t result = {};

    // Get information about the swapchain image that OpenXR made for us!
    XrSwapchainImageD3D12KHR& d3d_swapchain_img = (XrSwapchainImageD3D12KHR&)swapchain_img;
    D3D12_TEXTURE2D_DESC color_desc;
    d3d_swapchain_img.texture->GetDesc(&color_desc);

    // Create a view resource for the swapchain image target that we can use to set up rendering.
    D3D12_RENDER_TARGET_VIEW_DESC target_desc = {};
    target_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    // NOTE: Why not use color_desc.Format? Check the notes over near the xrCreateSwapchain call!
    // Basically, the color_desc.Format of the OpenXR created swapchain is TYPELESS, but in order to
    // create a View for the texture, we need a concrete variant of the texture format like UNORM.
    target_desc.Format = (DXGI_FORMAT)d3d_swapchain_fmt;
    d3d_device->CreateRenderTargetView(d3d_swapchain_img.texture, &target_desc, &result.target_view);

    // Create a depth buffer that matches
    ID3D12Texture2D* depth_texture;
    D3D12_TEXTURE2D_DESC depth_desc = {};
    depth_desc.SampleDesc.Count = 1;
    depth_desc.MipLevels = 1;
    depth_desc.Width = color_desc.Width;
    depth_desc.Height = color_desc.Height;
    depth_desc.ArraySize = color_desc.ArraySize;
    depth_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    depth_desc.BindFlags = D3D12_BIND_SHADER_RESOURCE | D3D12_BIND_DEPTH_STENCIL;
    d3d_device->CreateTexture2D(&depth_desc, nullptr, &depth_texture);

    // And create a view resource for the depth buffer, so we can set that up for rendering to as well!
    D3D12_DEPTH_STENCIL_VIEW_DESC stencil_desc = {};
    stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
    d3d_device->CreateDepthStencilView(depth_texture, &stencil_desc, &result.depth_view);

    // We don't need direct access to the ID3D12Texture2D object anymore, we only need the view
    depth_texture->Release();

    return result;
}

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            void DeviceCompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder, IDXGIAdapterX* dxgiAdapter)
            {
                DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo;

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Device");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_reservedInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_residentInBytes = memoryInfo.CurrentUsage;
                }

                if (S_OK == dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memoryInfo))
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = Name("Host");
                    heapStats->m_memoryUsage.m_budgetInBytes = memoryInfo.Budget;
                    heapStats->m_memoryUsage.m_reservedInBytes = memoryInfo.CurrentReservation;
                    heapStats->m_memoryUsage.m_residentInBytes = memoryInfo.CurrentUsage;
                }
            }

            D3D12_RESOURCE_STATES GetRayTracingAccelerationStructureResourceState()
            {
                return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            }
        }

        void EnableD3DDebugLayer()
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
            }
        }

        void EnableGPUBasedValidation()
        {
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1))))
            {
                debugController1->SetEnableGPUBasedValidation(TRUE);
                debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
            }

            Microsoft::WRL::ComPtr<ID3D12Debug2> debugController2;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController2))))
            {
                debugController2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
            }
        }

        void EnableDebugDeviceFeatures(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device)
        {
            Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
            if (SUCCEEDED(dx12Device->QueryInterface(debugDevice.GetAddressOf())))
            {
                debugDevice->SetFeatureMask(D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING);
            }
        }

        void EnableBreakOnD3DError(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device)
        {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(dx12Device->QueryInterface(infoQueue.GetAddressOf())))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                //Un-comment this if you want to break on warnings too
                //infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            }
        }

        bool IsRunningWindows10_0_17763()
        {
            Platform::WindowsVersion windowsVersion;
            if (!Platform::GetWindowsVersion(&windowsVersion))
            {
                return false;
            }
            return windowsVersion.m_majorVersion == 10 && windowsVersion.m_minorVersion == 0 && windowsVersion.m_buildVersion == 17763;
        }

        void AddDebugFilters(Microsoft::WRL::ComPtr<ID3D12DeviceX>& dx12Device, RHI::ValidationMode validationMode)
        {
            AZStd::vector<D3D12_MESSAGE_SEVERITY> enabledSeverities;
            AZStd::vector<D3D12_MESSAGE_ID> disabledMessages;

            // These severities should be seen all the time
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
            enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);

            if (validationMode == RHI::ValidationMode::Verbose)
            {
                // Verbose only filters
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
            }

            // [GFX TODO][ATOM-4573] - We keep getting this warning when reading from query buffers on a job thread
            // while a command queue thread is submitting a command list that is using the same buffer, but in a
            // different region. We should add validation elsewhere to make sure that multi-threaded access continues to
            // be valid and possibly find a way to restore this warning to catch other cases that could be invalid.
            disabledMessages.push_back(D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED);

            // Windows build 10.0.17763 (AKA version 1809) has a bug where the D3D Debug layer throws the error COPY_DESCRIPTORS_INVALID_RANGES when it shouldn't.
            // This was fixed in subsequent builds, however, Amazon IT is still deploying this version to new machines as of the time this comment was written.
            if (IsRunningWindows10_0_17763())
            {
                disabledMessages.push_back(D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES);
            }

            // We disable these warnings as the our current implementation of Pipeline Library will trigger these warnings unknowingly. For example
            // it will always first try to load a pso from pipelinelibrary triggering D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND (for the first time) before storing the PSO in a library.
            // Similarly when we merge multiple pipeline libraries (in multiple threads) we may trigger D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME as it is possible to save
            // a PSO already saved in the main library. 
#if defined (AZ_DX12_USE_PIPELINE_LIBRARY)
            disabledMessages.push_back(D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND);
            disabledMessages.push_back(D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME);
#endif

            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(dx12Device->QueryInterface(infoQueue.GetAddressOf())))
            {
                D3D12_INFO_QUEUE_FILTER filter{ };
                filter.AllowList.NumSeverities = azlossy_cast<UINT>(enabledSeverities.size());
                filter.AllowList.pSeverityList = enabledSeverities.data();
                filter.DenyList.NumIDs = azlossy_cast<UINT>(disabledMessages.size());
                filter.DenyList.pIDList = disabledMessages.data();

                // Clear out the existing filters since we're taking full control of them
                infoQueue->PushEmptyStorageFilter();

                [[maybe_unused]] HRESULT addedOk = infoQueue->AddStorageFilterEntries(&filter);
                AZ_Assert(addedOk == S_OK, "D3DInfoQueue AddStorageFilterEntries failed");

                infoQueue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_MESSAGE, "D3D12 Debug Filters setup");
            }
        }

        RHI::ValidationMode GetValidationMode()
        {
            RHI::ValidationMode validationMode = RHI::ValidationMode::Disabled;
            RHI::FactoryManagerBus::BroadcastResult(validationMode, &RHI::FactoryManagerRequest::DetermineValidationMode);
            return validationMode;
        }

        RHI::ResultCode Device::InitSubPlatform(RHI::PhysicalDevice& physicalDeviceBase)
        {
#if defined(USE_NSIGHT_AFTERMATH)
            // Enable Nsight Aftermath GPU crash dump creation.
            // This needs to be done before the D3D device is created.
            m_gpuCrashTracker.EnableGPUCrashDumps();
#endif
            PhysicalDevice& physicalDevice = static_cast<PhysicalDevice&>(physicalDeviceBase);
            RHI::ValidationMode validationMode = GetValidationMode();

            if (validationMode != RHI::ValidationMode::Disabled)
            {
                EnableD3DDebugLayer();
                if (validationMode == RHI::ValidationMode::GPU)
                {
                    EnableGPUBasedValidation();
                }
            }

            Microsoft::WRL::ComPtr<ID3D12DeviceX> dx12Device;
            if (FAILED(D3D12CreateDevice(physicalDevice.GetAdapter(), D3D_FEATURE_LEVEL_12_0, IID_GRAPHICS_PPV_ARGS(dx12Device.GetAddressOf()))))
            {
                AZ_Error("Device", false, "Failed to initialize the device. Check the debug layer for more info.");
                return RHI::ResultCode::Fail;
            }

            if (validationMode != RHI::ValidationMode::Disabled)
            {
                EnableDebugDeviceFeatures(dx12Device);
                EnableBreakOnD3DError(dx12Device);
                AddDebugFilters(dx12Device, validationMode);
            }

            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings))))
            {
                // Turn on auto-breadcrumbs and page fault reporting.
                pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }

            m_dx12Device = dx12Device.Get();
            m_dxgiFactory = physicalDevice.GetFactory();
            m_dxgiAdapter = physicalDevice.GetAdapter();
            
            InitDeviceRemovalHandle();

            m_isAftermathInitialized = Aftermath::InitializeAftermath(m_dx12Device);





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
            if(xr_instance == nullptr)
            {
                return RHI::ResultCode::Fail;
            }

            // Load extension methods that we'll need for this application
            xrGetInstanceProcAddr(
                xr_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
            xrGetInstanceProcAddr(
                xr_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
            xrGetInstanceProcAddr(
                xr_instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D12GraphicsRequirementsKHR));

            // Set up a really verbose debug log! Great for dev, but turn this off or
            // down for final builds. WMR doesn't produce much output here, but it
            // may be more useful for other runtimes?
            // Here's some extra information about the message types and severities:
            // https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#debug-message-categorization
            XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
            debug_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
            debug_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT /*severity*/,
                                         XrDebugUtilsMessageTypeFlagsEXT /*types*/,
                                         const XrDebugUtilsMessengerCallbackDataEXT* msg,
                                         void* /*user_data*/)
            {
                // Print the debug message we got! There's a bunch more info we could
                // add here too, but this is a pretty good start, and you can always
                // add a breakpoint this line!
                printf("%s: %s\n", msg->functionName, msg->message);

                // Output to debug window
                char text[512];
                sprintf_s(text, "%s: %s", msg->functionName, msg->message);
                OutputDebugStringA(text);

                // Returning XR_TRUE here will force the calling function to fail
                return (XrBool32)XR_FALSE;
            };
            // Start up the debug utils!
            if (ext_xrCreateDebugUtilsMessengerEXT)
            {
                ext_xrCreateDebugUtilsMessengerEXT(xr_instance, &debug_info, &xr_debug);
            }

            // Request a form factor from the device (HMD, Handheld, etc.)
            XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
            systemInfo.formFactor = app_config_form;
            xrGetSystem(xr_instance, &systemInfo, &xr_system_id);

            // Check what blend mode is valid for this device (opaque vs transparent displays)
            // We'll just take the first one available!
            uint32_t blend_count = 0;
            xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, app_config_view, 1, &blend_count, &xr_blend);

            // OpenXR wants to ensure apps are using the correct graphics card, so this MUST be called
            // before xrCreateSession. This is crucial on devices that have multiple graphics cards,
            // like laptops with integrated graphics chips in addition to dedicated graphics cards.
            XrGraphicsRequirementsD3D12KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
            ext_xrGetD3D12GraphicsRequirementsKHR(xr_instance, xr_system_id, &requirement);

            // A session represents this application's desire to display things! This is where we hook up our graphics API.
            // This does not start the session, for that, you'll need a call to xrBeginSession, which we do in openxr_poll_events
            XrGraphicsBindingD3D12KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
            binding.device = GetDevice();

            XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
            sessionInfo.next = &binding;
            sessionInfo.systemId = xr_system_id;
            xrCreateSession(xr_instance, &sessionInfo, &xr_session);

            // Unable to start a session, may not have an MR device attached or ready
            if (xr_session == nullptr)
            {
                return RHI::ResultCode::Fail;
            }

            // OpenXR uses a couple different types of reference frames for positioning content, we need to choose one for
            // displaying our content! STAGE would be relative to the center of your guardian system's bounds, and LOCAL
            // would be relative to your device's starting location. HoloLens doesn't have a STAGE, so we'll use LOCAL.
            XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
            ref_space.poseInReferenceSpace = xr_pose_identity;
            ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);

            // Now we need to find all the viewpoints we need to take care of! For a stereo headset, this should be 2.
            // Similarly, for an AR phone, we'll need 1, and a VR cave could have 6, or even 12!
            uint32_t view_count = 0;
            xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &view_count, nullptr);
            xr_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
            xr_views.resize(view_count, { XR_TYPE_VIEW });
            xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, view_count, &view_count, xr_config_views.data());
            for (uint32_t i = 0; i < view_count; i++)
            {
                // Create a swapchain for this viewpoint! A swapchain is a set of texture buffers used for displaying to screen,
                // typically this is a backbuffer and a front buffer, one for rendering data to, and one for displaying on-screen.
                // A note about swapchain image format here! OpenXR doesn't create a concrete image format for the texture, like
                // DXGI_FORMAT_R8G8B8A8_UNORM. Instead, it switches to the TYPELESS variant of the provided texture format, like
                // DXGI_FORMAT_R8G8B8A8_TYPELESS. When creating an ID3D12RenderTargetView for the swapchain texture, we must specify
                // a concrete type like DXGI_FORMAT_R8G8B8A8_UNORM, as attempting to create a TYPELESS view will throw errors, so
                // we do need to store the format separately and remember it later.
                XrViewConfigurationView& view = xr_config_views[i];
                XrSwapchainCreateInfo swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
                XrSwapchain handle;
                swapchain_info.arraySize = 1;
                swapchain_info.mipCount = 1;
                swapchain_info.faceCount = 1;
                swapchain_info.format = d3d_swapchain_fmt;
                swapchain_info.width = view.recommendedImageRectWidth;
                swapchain_info.height = view.recommendedImageRectHeight;
                swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
                swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                xrCreateSwapchain(xr_session, &swapchain_info, &handle);

                // Find out how many textures were generated for the swapchain
                uint32_t surface_count = 0;
                xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

                // We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
                // a depth buffer for each generated texture here as well with make_surfacedata.
                swapchain_t swapchain = {};
                swapchain.width = swapchain_info.width;
                swapchain.height = swapchain_info.height;
                swapchain.handle = handle;
                swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR });
                swapchain.surface_data.resize(surface_count);
                xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());
                for (uint32_t i = 0; i < surface_count; i++)
                {
                    swapchain.surface_data[i] = d3d_make_surface_data(GetDevice(), (XrBaseInStructure&)swapchain.surface_images[i]);
                }
                xr_swapchains.push_back(swapchain);
            }


            return RHI::ResultCode::Success;
        }

        void Device::ShutdownSubPlatform()
        {
            UnregisterWait(m_waitHandle);
            m_deviceFence.reset();

#ifdef AZ_DEBUG_BUILD
            ID3D12DebugDevice* dx12DebugDevice = nullptr;
            if (m_dx12Device)
            {
                m_dx12Device->QueryInterface(&dx12DebugDevice);
            }
            if (dx12DebugDevice)
            {
                dx12DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                dx12DebugDevice->Release();
            }
#endif
        }

        const char* GetBreadcrumpOpString(D3D12_AUTO_BREADCRUMB_OP op)
        {
            switch (op)
            {
            case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
                return "D3D12_AUTO_BREADCRUMB_OP_SETMARKER";
            case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
                return "D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT";
            case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
                return "D3D12_AUTO_BREADCRUMB_OP_ENDEVENT";
            case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
                return "D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED";
            case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
                return "D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT";
            case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
                return "D3D12_AUTO_BREADCRUMB_OP_DISPATCH";
            case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION";
            case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION";
            case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE";
            case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYTILES";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:
                return "D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW";
            case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE";
            case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
                return "D3D12_AUTO_BREADCRUMB_OP_PRESENT";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA";
            case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
                return "D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION";
            case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
                return "D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME";
            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:
                return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES";
            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:
                return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT";
            case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:
                return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION";
            case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:
                return "D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1";
            case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:
                return "D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION";
            case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:
                return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2";
            case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:
                return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1";
            case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:
                return "D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE";
            case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO:
                return "D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
            case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:
                return "D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE";
            case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:
                return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS";
            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:
                return "D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION";
            case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:
                return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP";
            case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:
                return "D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1";
            case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND";
            case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:
                return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND";

            // Disable due to the current minimum windows version doesn't have this enum
            // case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:
                // return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH";
            }
            return "unkown op";
        }

        bool Device::AssertSuccess(HRESULT hr)
        {
            if (hr == DXGI_ERROR_DEVICE_REMOVED)
            {
                OnDeviceRemoved();
            }

            bool success = SUCCEEDED(hr);
            AZ_Assert(success, "HRESULT not a success %x", hr);
            return success;
        }

        void Device::OnDeviceRemoved()
        {
            // It's possible this function is called many times at same time from different threads.
            // We want the other threads are blocked until the device removal is fully handled. 
            AZStd::lock_guard<AZStd::mutex> lock(m_onDeviceRemovedMutex);

            if (m_onDeviceRemoved)
            {
                return;
            }
            m_onDeviceRemoved = true;

            ID3D12Device* removedDevice = m_dx12Device.get();
            HRESULT removedReason = removedDevice->GetDeviceRemovedReason();
            
            AZ_TracePrintf("Device", "Device was removed because of the following reason:\n");

            switch (removedReason)
            {
            case DXGI_ERROR_DEVICE_HUNG:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_HUNG - The application's device failed due to badly formed commands sent by the "
                    "application. This is an design-time issue that should be investigated and fixed.\n");
                break;
            case DXGI_ERROR_DEVICE_REMOVED:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_REMOVED - The video card has been physically removed from the system, or a driver upgrade "
                    "for the video card has occurred. The application should destroy and recreate the device. For help debugging "
                    "the problem, call ID3D10Device::GetDeviceRemovedReason.\n");
                break;
            case DXGI_ERROR_DEVICE_RESET:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DEVICE_RESET - The device failed due to a badly formed command. This is a run-time issue; The "
                    "application should destroy and recreate the device.\n");
                break;
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_DRIVER_INTERNAL_ERROR - The driver encountered a problem and was put into the device removed "
                    "state.\n");
                break;
            case DXGI_ERROR_INVALID_CALL:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_INVALID_CALL - The application provided invalid parameter data; this must be debugged and fixed "
                    "before the application is released.\n");
                break;
            case DXGI_ERROR_ACCESS_DENIED:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI_ERROR_ACCESS_DENIED - You tried to use a resource to which you did not have the required access "
                    "privileges. This error is most typically caused when you write to a shared resource with read-only access.\n");
                break;
            case S_OK:
                AZ_TracePrintf("DX12", "S_OK - The method succeeded without an error.\n");
                break;
            default:
                AZ_TracePrintf(
                    "DX12",
                    "DXGI error code: %X\n", removedReason);
                break;
            }
           
            // Perform app-specific device removed operation, such as logging or inspecting DRED output
            Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData> pDred;
            if (SUCCEEDED(removedDevice->QueryInterface(IID_PPV_ARGS(&pDred))))
            {
                D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT dredAutoBreadcrumbsOutput;

                if (SUCCEEDED(pDred->GetAutoBreadcrumbsOutput(&dredAutoBreadcrumbsOutput)))
                {
                    auto* currentNode = dredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
                    while (currentNode)
                    {
                        bool hasError = false;
                        bool isWide = currentNode->pCommandListDebugNameW;
                        uint32_t completedBreadcrumbCount = currentNode->pLastBreadcrumbValue? (*currentNode->pLastBreadcrumbValue):0;
                        if (completedBreadcrumbCount < currentNode->BreadcrumbCount && completedBreadcrumbCount > 0)
                        {
                            AZ_TracePrintf("Device", "[Error]");
                            hasError = true;
                        }
                        AZStd::string info;
                        if (isWide)
                        {
                            info = AZStd::string::format(
                                "CommandList name: [%S] address [%p] CommandQueue name: [%S] address [%p] BreadcrumbCount: %d Completed BreadcrumbCount %d \n",
                                currentNode->pCommandListDebugNameW, currentNode->pCommandList, currentNode->pCommandQueueDebugNameW,
                                currentNode->pCommandQueue, currentNode->BreadcrumbCount, completedBreadcrumbCount);
                        }
                        else
                        {
                            info = AZStd::string::format(
                                "CommandList name: [%s] address [%p] CommandQueue name: [%s] address [%p] BreadcrumbCount: %d  Completed BreadcrumbCount %d\n",
                                currentNode->pCommandListDebugNameA, currentNode->pCommandList, currentNode->pCommandQueueDebugNameA,
                                currentNode->pCommandQueue, currentNode->BreadcrumbCount, completedBreadcrumbCount);
                        }

                        AZ_TracePrintf("Device", info.c_str());

                        AZ_TracePrintf("Device", "Context\n");
                        
                        for (uint32_t index = 0; index < currentNode->BreadcrumbCount; index++)
                        {
                            if (hasError && index == completedBreadcrumbCount)
                            {
                                // where the error happened
                                AZ_TracePrintf("Device", " ==========================Error start==================================\n");
                            }
                            AZ_TracePrintf("Device", "      %d %s\n", index, GetBreadcrumpOpString(currentNode->pCommandHistory[index]));
                            if (hasError && index == completedBreadcrumbCount)
                            {
                                // where the error happened
                                AZ_TracePrintf("Device", " ==========================Error end=============================\n");
                            }
                        }
                        
                        AZ_TracePrintf("Device", " ==================================================================\n");

                        currentNode = currentNode->pNext;
                    }
                }
            }

            AZ_TracePrintf("Device", " ===========================End of OnDeviceRemoved================================\n");

            if (IsAftermathInitialized())
            {
                // Try outputting the name of the last scope that was executing on the GPU
                // There is a good chance that is the cause of the GPU crash and should be investigated first
                Aftermath::OutputLastScopeExecutingOnGPU(GetAftermathGPUCrashTracker());
            }

            SetDeviceRemoved();
        }

        void HandleDeviceRemoved(PVOID context, BOOLEAN)
        {
            Device* removedDevice = (Device*)context;
            removedDevice->OnDeviceRemoved();
        }

        void Device::InitDeviceRemovalHandle()
        {
            // Create fence to detect device removal
            Microsoft::WRL::ComPtr<ID3D12Fence> fencePtr;
            if (FAILED(m_dx12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fencePtr.GetAddressOf()))))
            {
                return;
            }
            m_deviceFence = fencePtr.Get();
            HANDLE deviceRemovedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
            m_deviceFence->SetEventOnCompletion(UINT64_MAX, deviceRemovedEvent);

            RegisterWaitForSingleObject(
              &m_waitHandle,
              deviceRemovedEvent,
              HandleDeviceRemoved,
              this, // Pass the device as our context
              INFINITE, // No timeout
              0 // No flags
            );
        }

        RHI::ResultCode Device::CreateSwapChain(
            IUnknown* window,
            const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
            RHI::Ptr<IDXGISwapChainX>& outSwapChain)
        {
            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChainPtr;

            HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
                m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Graphics).GetPlatformQueue(),
                reinterpret_cast<HWND>(window),
                &swapChainDesc,
                nullptr,
                nullptr,
                &swapChainPtr);

            if (FAILED(hr))
            {
                _com_error error(hr);
                AZ_Error("Device", false, "Failed to initialize SwapChain with error 0x%x(%s) Check the debug layer for more info.\nDimensions: %u x %u DXGI_FORMAT: %u", hr, error.ErrorMessage(), swapChainDesc.Width, swapChainDesc.Height, swapChainDesc.Format);
                return RHI::ResultCode::Fail;
            }

            Microsoft::WRL::ComPtr<IDXGISwapChainX> swapChainX;
            swapChainPtr->QueryInterface(IID_GRAPHICS_PPV_ARGS(swapChainX.GetAddressOf()));

            outSwapChain = swapChainX.Get();
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Device::CreateSwapChain(
            [[maybe_unused]] const DXGI_SWAP_CHAIN_DESCX& swapChainDesc,
            [[maybe_unused]] AZStd::array<RHI::Ptr<ID3D12Resource>, RHI::Limits::Device::FrameCountMax>& outSwapChainResources)
        {
            AZ_Assert(false, "Wrong Device::CreateSwapChain function called on Windows.");
            return RHI::ResultCode::Fail;
        }

        AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        {
            AZStd::vector<RHI::Format> formatsList;

            // Follows Microsoft's HDR sample code for determining if the connected display supports HDR.
            // Enumerates all of the detected displays and determines which one has the largest intersection with the
            // region of the window handle parameter.
            // If the display for this region supports wide color gamut, then a wide color gamut format is added to
            // the list of supported formats.
            // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/UWP/D3D12HDR/src/D3D12HDR.cpp

            HWND hWnd = reinterpret_cast<HWND>(windowHandle.GetIndex());
            RECT windowRect = {};
            GetWindowRect(hWnd, &windowRect);

            UINT outputIndex = 0;
            Microsoft::WRL::ComPtr<IDXGIOutput> bestOutput;
            Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;
            RECT intersectRect;
            int bestIntersectionArea = -1;
            while (m_dxgiAdapter->EnumOutputs(outputIndex, &currentOutput) == S_OK)
            {
                // Get the rectangle bounds of current output
                DXGI_OUTPUT_DESC outputDesc;
                currentOutput->GetDesc(&outputDesc);
                RECT outputRect = outputDesc.DesktopCoordinates;
                int intersectionArea = 0;
                if (IntersectRect(&intersectRect, &windowRect, &outputRect))
                {
                    intersectionArea = (intersectRect.bottom - intersectRect.top) * (intersectRect.right - intersectRect.left);
                }
                if (intersectionArea > bestIntersectionArea)
                {
                    bestOutput = currentOutput;
                    bestIntersectionArea = intersectionArea;
                }

                outputIndex++;
            }

            if (bestOutput)
            {
                Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
                [[maybe_unused]] HRESULT hr = bestOutput.As(&output6);
                AZ_Assert(S_OK == hr, "Failed to get IDXGIOutput6 structure.");
                DXGI_OUTPUT_DESC1 outputDesc;
                output6->GetDesc1(&outputDesc);
                if (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                {
                    // HDR is supported
                    formatsList.push_back(RHI::Format::R10G10B10A2_UNORM);
                }
            }

            // Fallback default 8-bit format
            formatsList.push_back(RHI::Format::R8G8B8A8_UNORM);

            return formatsList;
        }

        void Device::BeginFrameInternal()
        {
            m_commandQueueContext.Begin();
        }
    }
}

