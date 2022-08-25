/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/TestImpactProcessException.h>
#include <Process/TestImpactProcessInfo.h>

namespace TestImpact
{
    ProcessInfo::ProcessInfo(ProcessId id, const RepoPath& processPath, const AZStd::string& startupArgs)
        : m_id(id)
        , m_processPath(processPath)
        , m_startupArgs(startupArgs)
    {
        AZ_TestImpact_Eval(processPath.String().length() > 0, ProcessException, "Process path cannot be empty");
    }

    ProcessInfo::ProcessInfo(
        ProcessId id,
        AZStd::variant<StdOutputRouting, AZ::IO::Path>&& stdOut,
        AZStd::variant<StdErrorRouting, AZ::IO::Path>&& stdErr,
        const RepoPath& processPath,
        const AZStd::string& startupArgs)
        : m_id(id)
        , m_processPath(processPath)
        , m_startupArgs(startupArgs)
    {
        AZ_TestImpact_Eval(processPath.String().length() > 0, ProcessException, "Process path cannot be empty");

        AZStd::visit(
            [&](auto&& std)
            {
                if constexpr (AZStd::is_same_v<StdOutputRouting, AZStd::decay_t<decltype(std)>>)
                {
                    m_stdOutputRouting = std;
                }
                else
                {
                    m_stdOutputFilePath = std::move(std);
                }
            }, stdOut);

        AZStd::visit(
            [&](auto&& std)
            {
                if constexpr (AZStd::is_same_v<StdErrorRouting, AZStd::decay_t<decltype(std)>>)
                {
                    m_stdErrorRouting = std;
                }
                else
                {
                    m_stdErrorFilePath = std::move(std);
                }
            }, stdErr);
    }

    ProcessId ProcessInfo::GetId() const
    {
        return m_id;
    }

    const RepoPath& ProcessInfo::GetProcessPath() const
    {
        return m_processPath;
    }

    const AZStd::string& ProcessInfo::GetStartupArgs() const
    {
        return m_startupArgs;
    }

    bool ProcessInfo::ParentHasStdOutput() const
    {
        return StdOutputRouting::ToParent == m_stdOutputRouting;
    }

    bool ProcessInfo::ParentHasStdError() const
    {
        return StdErrorRouting::ToParent == m_stdErrorRouting;
    }

    bool ProcessInfo::FileHasStdOutput() const
    {
        return !m_stdOutputFilePath.empty();
    }

    bool ProcessInfo::FileHasStdError() const
    {
        return !m_stdErrorFilePath.empty();
    }

    const AZ::IO::Path& ProcessInfo::GetStdOutputFile() const
    {
        return m_stdOutputFilePath;
    }

    const AZ::IO::Path& ProcessInfo::GetStdErrorFile() const
    {
        return m_stdErrorFilePath;
    }
} // namespace TestImpact
