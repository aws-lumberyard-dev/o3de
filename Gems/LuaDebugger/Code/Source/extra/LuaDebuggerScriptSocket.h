/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Socket/AzSocket.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
    class ScriptDataContext;
}

namespace LuaDebugger
{
#if defined(AZ_DEBUG_BUILD)

    // LuaDebuggerScriptSocket class    
    class LuaDebuggerScriptSocket
    {
    public:
        // --- Construction/Initialization
        AZ_TYPE_INFO(LuaDebuggerScriptSocket, "{65437353-CB60-4F78-9D77-9BCA5D39B5C4}");
        AZ_CLASS_ALLOCATOR(LuaDebuggerScriptSocket, AZ::SystemAllocator, 0);

        LuaDebuggerScriptSocket();
        LuaDebuggerScriptSocket(const LuaDebuggerScriptSocket&) = delete;
        LuaDebuggerScriptSocket(LuaDebuggerScriptSocket&&);
        LuaDebuggerScriptSocket& operator=(const LuaDebuggerScriptSocket&) = delete;
        LuaDebuggerScriptSocket& operator=(LuaDebuggerScriptSocket&&);
        ~LuaDebuggerScriptSocket();

        static void Reflect(AZ::ReflectContext* context);

    private:
        // --- Private Member Functions ---

        // arg 1 = host name, arg 2 = port
        // on success, returns 1 and an empty error string
        // on failure, returns nil and an error string
        void Connect(AZ::ScriptDataContext& dc);
        // arg1 = string data to send
        // on success, returns number of bytes sent and an empty error string
        // on failure, returns nil and an error string
        void SendString(AZ::ScriptDataContext& dc);
        // Reads from the socket until an newline is encountered
        // on success, returns string data with \r and \n stripped, and an empty error string
        // on failure, returns nil and and an error string
        void ReceiveLine(AZ::ScriptDataContext& dc);
        // Sets the timeout to use for subsequent calls to SendString/ReceiveLine
        void SetTimeoutInSecs(double secs);
        // Closes the socket connection
        void Close();

        static LuaDebuggerScriptSocket* CreateSocket();

        // --- Private Member Data ---

        AZSOCKET m_socket;
        AZTIMEVAL m_timeout;
        AZStd::string m_received;
    };
#endif
}
