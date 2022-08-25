/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/variant.h>

namespace TestImpact
{
    //! Identifier to distinguish between processes.
    using ProcessId = size_t;

    //! Return code of successfully launched process.
    using ReturnCode = int;

    //! Error code for processes that are forcefully terminated whilst in-flight by the client.
    inline constexpr const ReturnCode ProcessTerminateErrorCode = 0xF10BAD;

    //! Error code for processes that are forcefully terminated whilst in-flight by the scheduler due to timing out.
    inline constexpr const ReturnCode ProcessTimeoutErrorCode = 0xBADF10;

    //! Specifier for how the process's standard out will be routed
    enum class StdOutputRouting
    {
        ToParent,
        None
    };

    enum class StdErrorRouting
    {
        ToParent,
        None
    };

    //! Container for process standard output and standard error.
    struct StdContent
    {
        AZStd::optional<AZStd::string> m_out;
        AZStd::optional<AZStd::string> m_err;
    };

    //! Information about a process the arguments used to launch it.
    class ProcessInfo
    {
    public:
        //! Provides the information required to launch a process.
        //! @param processId Client-supplied id to diffrentiate between processes.
        //! @param stdOut Routing of process standard output.
        //! @param stdErr Routing of process standard error.
        //! @param processPath Path to executable binary to launch.
        //! @param startupArgs Arguments to launch the process with.
        ProcessInfo(
            ProcessId processId,
            AZStd::variant<StdOutputRouting, AZ::IO::Path>&& stdOut,
            AZStd::variant<StdErrorRouting, AZ::IO::Path>&& stdErr,
            const RepoPath& processPath,
            const AZStd::string& startupArgs = "");
        ProcessInfo(ProcessId processId, const RepoPath& processPath, const AZStd::string& startupArgs = "");

        //! Returns the identifier of this process.
        ProcessId GetId() const;

        //! Returns whether or not stdoutput is routed to the parent process.
        bool ParentHasStdOutput() const;

        //! Returns whether or not stderror is routed to the parent process.
        bool ParentHasStdError() const;

        //! Returns whether or not stdoutput is routed to a file.
        bool FileHasStdOutput() const;

        //! Returns whether or not stderror is routed to a file.
        bool FileHasStdError() const;

        //! Returns the path to the file that the stdoutput is routed to.
        const AZ::IO::Path& GetStdOutputFile() const;

        //! Returns the path to the file that the stderror is routed to.
        const AZ::IO::Path& GetStdErrorFile() const;

        // Returns the path to the process binary.
        const RepoPath& GetProcessPath() const;

        //! Returns the command line arguments used to launch the process.
        const AZStd::string& GetStartupArgs() const;

    private:
        ProcessId m_id;
        StdOutputRouting m_stdOutputRouting = StdOutputRouting::None;
        StdErrorRouting m_stdErrorRouting = StdErrorRouting::None;
        AZ::IO::Path m_stdOutputFilePath;
        AZ::IO::Path m_stdErrorFilePath;
        RepoPath m_processPath;
        AZStd::string m_startupArgs;
    };
} // namespace TestImpact
