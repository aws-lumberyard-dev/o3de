/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

#include <AzCore/base.h>

#include <AzCore/Memory/AllocationRecords.h>

// the full function template is required due to the print function being defaulted.
#include <AzCore/std/function/function_template.h>


namespace AZ::Debug
{
    using PrintFunction = AZStd::function<void (const char*)>;
    /**
    * Example of records enumeration callback.
    */
    struct PrintAllocationsCB
    {
        // Print out allocations
        // If you supply an optional printing callback, it will send each line to your function.
        // Otherwise it will call AZ_Printf.
        PrintAllocationsCB(bool isDetailed = false, bool includeNameAndFilename = false, PrintFunction printer = {})
            : m_isDetailed(isDetailed), m_includeNameAndFilename(includeNameAndFilename), m_printFunction(printer) {}

        bool operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels);
        PrintFunction m_printFunction;
        bool m_isDetailed;      ///< True to print allocation line and allocation callstack, otherwise false.
        bool m_includeNameAndFilename;  /// < True to print the source name and source filename, otherwise skip
    };
}
