/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include<ScriptAutomationUtils.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace ScriptAutomation
{
    namespace Utils
    {
        bool ResizeClientArea(uint32_t width, uint32_t height)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            AzFramework::WindowSize clientAreaSize = {width, height};
            AzFramework::WindowRequestBus::Event(windowHandle, &AzFramework::WindowRequestBus::Events::ResizeClientArea, clientAreaSize);
            AzFramework::WindowSize newWindowSize;
            AzFramework::WindowRequestBus::EventResult(newWindowSize, windowHandle, &AzFramework::WindowRequests::GetClientAreaSize);

            if (newWindowSize.m_width == width && newWindowSize.m_height == height)
            {
                return true;
            }
            else
            {
                AZ_Error("ScriptAutomation", false,
                    "Requested window resize to %ux%u but got %ux%u. This display resolution is too low or desktop scaling is too high.",
                    width, height, newWindowSize.m_width, newWindowSize.m_height);
            }

            return false;
        }
    }
}