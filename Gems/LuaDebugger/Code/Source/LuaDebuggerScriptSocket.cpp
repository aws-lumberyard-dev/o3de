/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <LuaDebugger/LuaDebuggerScriptSocket.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace LuaDebugger
{
    // LuaDebuggerScriptSocket Methods
    LuaDebuggerScriptSocket::LuaDebuggerScriptSocket()
    {
        AZ::AzSock::Startup();
        m_socket = AZ::AzSock::Socket();
        AZ_Error("LuaDebuggerScriptSocket", AZ::AzSock::IsAzSocketValid(m_socket), "Invalid socket created");

        AZ::AzSock::SetSocketBlockingMode(m_socket, true);
        AZ::AzSock::SetSocketOption(m_socket, AZ::AzSock::AzSocketOption::KEEPALIVE, true);

        m_timeout.tv_sec = 0;
        m_timeout.tv_usec = 0;
    }

    LuaDebuggerScriptSocket::~LuaDebuggerScriptSocket()
    {
        Close();
        AZ::AzSock::Cleanup();
    }

    LuaDebuggerScriptSocket::LuaDebuggerScriptSocket(LuaDebuggerScriptSocket&& other)
        : m_socket(other.m_socket)
        , m_timeout(other.m_timeout)
    {
        other.m_socket = AZ_SOCKET_INVALID;
    }

    LuaDebuggerScriptSocket& LuaDebuggerScriptSocket::operator=(LuaDebuggerScriptSocket&& rhs)
    {
        if (this != &rhs)
        {
            m_socket = rhs.m_socket;
            rhs.m_socket = AZ_SOCKET_INVALID;
            m_timeout = rhs.m_timeout;
        }
        return *this;
    }

    void LuaDebuggerScriptSocket::Reflect(AZ::ReflectContext* context)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<LuaDebuggerScriptSocket>("Socket")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("CreateSocket", &LuaDebuggerScriptSocket::CreateSocket)
                ->Method("Connect", &LuaDebuggerScriptSocket::Connect)
                ->Method("SendString", &LuaDebuggerScriptSocket::SendString)
                ->Method("ReceiveLine", &LuaDebuggerScriptSocket::ReceiveLine)
                ->Method("SetTimeoutInSecs", &LuaDebuggerScriptSocket::SetTimeoutInSecs)
                ->Method("Close", &LuaDebuggerScriptSocket::Close)
                ;
        }
    }

    void LuaDebuggerScriptSocket::Connect(AZ::ScriptDataContext& dc)
    {
        const char* host = nullptr;
        AZ::u16 port = 0;
        if (dc.GetNumArguments() != 2 || !dc.ReadArg(0, host) || !dc.ReadArg(1, port))
        {
            dc.PushResult(false);
            dc.PushResult<const char*>("Invalid Arguments");
            return;
        }

#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
        AZ_Printf("LuaDebuggerScriptSocket", "Connect socket to %s:%u", host, port);
#endif
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::AzSocketAddress addr;
            addr.SetAddress(host, port);
            AZ::s32 result = AZ::AzSock::Connect(m_socket, addr);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                dc.PushResult<const char*>(nullptr);
                dc.PushResult(AZ::AzSock::GetStringForError(result));

                AZ_Error("LuaDebuggerScriptSocket", false, "Socket connect error: '%s'", AZ::AzSock::GetStringForError(result));
            }
            else
            {
                dc.PushResult(1);
                dc.PushResult<const char*>("");
            }
        }
        else
        {
            dc.PushResult<const char*>(nullptr);
            dc.PushResult<const char*>("Invalid Socket");
        }
    }

    void LuaDebuggerScriptSocket::SendString(AZ::ScriptDataContext& dc)
    {
        const char* data = nullptr;
        if (dc.GetNumArguments() != 1 || !dc.ReadArg(0, data))
        {
            dc.PushResult<const char*>(nullptr);
            dc.PushResult<const char*>("Invalid Arguments");
            return;
        }

#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
        AZ_Printf("LuaDebuggerScriptSocket", "Send '%s' to socket", data);
#endif
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::s32 writableResult = AZ::AzSock::WaitForWritableSocket(m_socket, &m_timeout);
            if (writableResult > 0)
            {
                AZ::s32 numBytes = static_cast<AZ::s32>(strlen(data));
                AZ::s32 sendResult = AZ::AzSock::Send(m_socket, data, numBytes, 0);
                if (AZ::AzSock::SocketErrorOccured(sendResult))
                {
                    dc.PushResult<const char*>(nullptr);
                    dc.PushResult(AZ::AzSock::GetStringForError(sendResult));
                    AZ_Error("ScriptSocket", false, "Send error: '%s'", AZ::AzSock::GetStringForError(sendResult));
                }
                else
                {
                    dc.PushResult(numBytes);
                    dc.PushResult<const char*>("");
                }
            }
            else
            {
                dc.PushResult<const char*>(nullptr);
                dc.PushResult<const char*>("Timeout");
            }
        }
        else
        {
            dc.PushResult<const char*>(nullptr);
            dc.PushResult<const char*>("Invalid Socket");
        }
    }

    void LuaDebuggerScriptSocket::ReceiveLine(AZ::ScriptDataContext& dc)
    {
#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
        AZ_Printf("LuaDebuggerScriptSocket", "Receive to socket");
#endif
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            auto processReceived = [this, &dc]()
            {
                size_t newLinePos = m_received.find('\n');
                if (newLinePos != AZStd::string::npos)
                {
                    AZStd::string resultStr = m_received.substr(0, newLinePos);
                    m_received.erase(0, newLinePos + 1);
                    if (!resultStr.empty() && resultStr[resultStr.length() - 1] == '\r')
                    {
                        resultStr.pop_back();
                    }

                    dc.PushResult(resultStr);
                    dc.PushResult<const char*>("");

#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
                    AZ_Printf("LuaDebuggerScriptSocket", "Received '%s' to socket", resultStr.c_str());
#endif
                    return true;
                }

                return false;
            };

            while (!processReceived())
            {
                AZ::s32 pendingResult = AZ::AzSock::IsRecvPending(m_socket, &m_timeout);
                if (pendingResult > 0)
                {
                    char buf[1024];
                    AZ::s32 recvResult = AZ::AzSock::Recv(m_socket, buf, AZ_ARRAY_SIZE(buf), 0);
                    if (AZ::AzSock::SocketErrorOccured(recvResult))
                    {
                        dc.PushResult<const char*>(nullptr);
                        dc.PushResult(AZ::AzSock::GetStringForError(recvResult));
                        AZ_Error("VsCodeLuaDebuggerScriptSocket", false, "Receive error: '%s'", AZ::AzSock::GetStringForError(recvResult));
                        break;
                    }
                    else
                    {
                        AZ::s32 strLength = recvResult;
                        m_received.append(buf, strLength);
                    }
                }
                else
                {
                    dc.PushResult<const char*>(nullptr);
                    dc.PushResult<const char*>("Timeout");
                    break;
                }
            }
        }
        else
        {
            dc.PushResult<const char*>(nullptr);
            dc.PushResult<const char*>("Invalid Socket");
        }
    }

    void LuaDebuggerScriptSocket::SetTimeoutInSecs(double secs)
    {
#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
        AZ_Printf("LuaDebuggerScriptSocket", "Set socket timeout: %f secs", secs);
#endif
        AZ::u64 timeoutUs = static_cast<AZ::u64>(secs * 1000000);
        m_timeout.tv_sec = static_cast<long>(timeoutUs / 1000000);
        m_timeout.tv_usec = static_cast<long>(timeoutUs % 1000000);
    }

    void LuaDebuggerScriptSocket::Close()
    {
#ifdef SCRIPT_SOCKET_VERBOSE_LOGGING
        AZ_Printf("LuaDebuggerScriptSocket", "Close socket");
#endif
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
            m_socket = AZ_SOCKET_INVALID;
        }
    }

    LuaDebuggerScriptSocket* LuaDebuggerScriptSocket::CreateSocket()
    {
        return aznew LuaDebuggerScriptSocket{};
    }
}
