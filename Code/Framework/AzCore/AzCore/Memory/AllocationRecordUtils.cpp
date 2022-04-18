/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Memory/AllocationRecordUtils.h>
#include <AzCore/Debug/StackTracer.h>

namespace AZ::Debug
{
    bool PrintAllocationsCB::operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels)
    {
        // do our best to avoid allocating memory on the heap when dumping allocations which could be in the heap!
        char staticBuffer[2048];
        if (!m_printFunction)
        {
            m_printFunction = [](const char* buffer)
            {
                AZ_Printf("Memory", "%s\n", buffer);
            };
        }

        if (m_includeNameAndFilename && info.m_name)
        {
            azsprintf(staticBuffer, "Allocation Name: \"%s\" Addr: 0%p Size: %zu Alignment: %d",
                      info.m_name,
                      address, info.m_byteSize,
                      static_cast<int>(info.m_alignment));
            staticBuffer[2047] = 0;

            m_printFunction(staticBuffer);
        }
        else
        {
            azsprintf(staticBuffer,"Allocation Addr: 0%p Size: %zu Alignment: %d", address, info.m_byteSize, static_cast<int>(info.m_alignment));
            m_printFunction(staticBuffer);
        }

        if (m_isDetailed)
        {
            if (!info.m_stackFrames)
            {
                azsprintf(staticBuffer," %s (%d)", info.m_fileName, info.m_lineNum);
                m_printFunction(staticBuffer);
            }
            else
            {
                // Allocation callstack
                const unsigned char decodeStep = 40;
                Debug::SymbolStorage::StackLine lines[decodeStep];
                unsigned char iFrame = 0;
                while (numStackLevels > 0)
                {
                    unsigned char numToDecode = AZStd::GetMin(decodeStep, numStackLevels);
                    Debug::SymbolStorage::DecodeFrames(&info.m_stackFrames[iFrame], numToDecode, lines);
                    for (unsigned char i = 0; i < numToDecode; ++i)
                    {
                        if (info.m_stackFrames[iFrame + i].IsValid())
                        {
                            azsprintf(staticBuffer," %s", lines[i]);
                            m_printFunction(staticBuffer);
                        }
                    }
                    numStackLevels -= numToDecode;
                    iFrame += numToDecode;
                }
            }
        }
        return true; // continue enumerating
    }
}
