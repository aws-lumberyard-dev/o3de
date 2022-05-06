/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/base.h>

namespace AZ
{
    namespace RPI
    {
        class XRDeviceDescriptor
        {
        public:
            AZ_RTTI(XRDeviceDescriptor, "{55E9010A-3EF2-4B12-A355-A8189BC4B0E7}");

            XRDeviceDescriptor() = default;
            virtual ~XRDeviceDescriptor() = default;

            AZStd::string m_description;
            AZ::u32 m_deviceId = 0;
        };

        class XRInstanceDescriptor
        {
        public:
            AZ_RTTI(XRInstanceDescriptor, "{FE1EC82F-6265-4A67-84D2-D05D4229B598}");

            XRInstanceDescriptor() = default;
            virtual ~XRInstanceDescriptor() = default;
        };

        class XRSwapChainImageDescriptor
        {
        public:
            AZ_RTTI(XRSwapChainImageDescriptor, "{E63273BF-BA94-431D-8174-69DD96A6CF94}");

            XRSwapChainImageDescriptor() = default;
            virtual ~XRSwapChainImageDescriptor() = default;
        };

        class XRGraphicsBindingDescriptor
        {
        public:
            AZ_RTTI(XRGraphicsBindingDescriptor, "{5F105FF8-CB52-4E6B-9511-7479F12939ED}");

            XRGraphicsBindingDescriptor() = default;
            virtual ~XRGraphicsBindingDescriptor() = default;
        };

        class XRSystemInterface
        {
        public:
            AZ_RTTI(XRSystemInterface, "{18177EAF-3014-4349-A28F-BF58442FFC2B}");

            XRSystemInterface() = default;
            virtual ~XRSystemInterface() = default;

            static XRSystemInterface* Get();

            // Creates the XR::Instance which is responsible for managing
            // XrInstance (amongst other things) for OpenXR backend
            // Also initializes the XR::Device
            virtual AZ::RHI::ResultCode InitializeSystem() = 0;

            // Create a Session and other basic session-level initialization.
            virtual AZ::RHI::ResultCode InitializeSession() = 0;

            // Start of the frame related XR work
            virtual void BeginFrame() = 0;

            // End of the frame related XR work
            virtual void EndFrame() = 0;

            // Start of the view related work
            virtual void BeginView() = 0;

            // End of the view related work
            virtual void EndView() = 0;

            // Manage session lifecycle to track if RenderFrame should be called.
            virtual bool IsSessionRunning() const = 0;

            // Create a Swapchain which will responsible for managing
            // multiple XR swapchains and multiple swapchain images within it
            virtual void CreateSwapchain() = 0;

            // This will allow XR gem to provide device related data to RHI
            virtual XRDeviceDescriptor* GetDeviceDescriptor() = 0;

            // Provide access to instance specific data to RHI
            virtual XRInstanceDescriptor* GetInstanceDescriptor() = 0;

            // Provide Swapchain specific data to RHI
            virtual XRSwapChainImageDescriptor* GetSwapChainImageDescriptor(int swapchainIndex) = 0;

            // Provide access to Graphics Binding specific data that RHI can populate
            virtual XRGraphicsBindingDescriptor* GetGraphicsBindingDescriptor() = 0;
        };
    } // namespace RPI
} // namespace AZ
