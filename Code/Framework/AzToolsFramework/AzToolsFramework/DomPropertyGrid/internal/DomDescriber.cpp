/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzToolsFramework/DomPropertyGrid/internal/DomDescriber.h>

namespace AzToolsFramework
{
    DomDescriber::DomDescriber(rapidjson::Value& rootValue)
        : m_rootValue(rootValue)
    {
    }

    void DomDescriber::DescribeInt8()
    {
        AZ::ScopedStackedString name(m_path, "int 8");
        AZ_TracePrintf("DomModel", "int 8: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeInt16()
    {
        AZ::ScopedStackedString name(m_path, "int 16");
        AZ_TracePrintf("DomModel", "int 16: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeInt32()
    {
        AZ::ScopedStackedString name(m_path, "int 32");
        AZ_TracePrintf("DomModel", "int 32: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeInt64()
    {
        AZ::ScopedStackedString name(m_path, "int 64");
        AZ_TracePrintf("DomModel", "int 64: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }

    void DomDescriber::DescribeUint8()
    {
        AZ::ScopedStackedString name(m_path, "uint 8");
        AZ_TracePrintf("DomModel", "uint 8: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeUint16()
    {
        AZ::ScopedStackedString name(m_path, "uint 16");
        AZ_TracePrintf("DomModel", "uint 16: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeUint32()
    {
        AZ::ScopedStackedString name(m_path, "uint 32");
        AZ_TracePrintf("DomModel", "uint 32: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeUint64()
    {
        AZ::ScopedStackedString name(m_path, "uint 64");
        AZ_TracePrintf("DomModel", "uint 64: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }

    void DomDescriber::DescribeFloat()
    {
        AZ::ScopedStackedString name(m_path, "float");
        AZ_TracePrintf("DomModel", "float: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }
    void DomDescriber::DescribeDouble()
    {
        AZ::ScopedStackedString name(m_path, "double");
        AZ_TracePrintf("DomModel", "double: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }

    void DomDescriber::DescribeString()
    {
        AZ::ScopedStackedString name(m_path, "string");
        AZ_TracePrintf("DomModel", "string: %.*s\n", AZ_STRING_ARG(m_path.Get()));
    }

    void DomDescriber::DescribeObjectBegin(AZStd::string_view type, [[maybe_unused]] const AZ::TypeId& typeId)
    {
        if (m_depth != 0)
        {
            m_path.Push(type);
        }
        AZ_TracePrintf("DomModel", "Object begin: %.*s\n", AZ_STRING_ARG(m_path.Get()));
        m_depth++;
    }
    void DomDescriber::DescribeObjectEnd()
    {
        AZ_TracePrintf("DomModel", "Object end: %.*s\n", AZ_STRING_ARG(m_path.Get()));
        m_depth--;
        if (m_depth != 0)
        {
            m_path.Pop(); 
        }
    }
} // namespace AzToolsFramework
