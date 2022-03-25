/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "ReplacementUtils.h"

namespace ScriptCanvas
{
    typedef std::tuple<std::string, std::string> MethodConfig;
    static std::unordered_map<std::string, MethodConfig> m_replacementMethods = {
        { "Entity Transform::Rotate", { "", "ScriptCanvas_EntityFunctions_Rotate" } },
        { "String::Split", { "", "ScriptCanvas_StringFunctions_Split" } },
        { "String::Join", { "", "ScriptCanvas_StringFunctions_Join" } },
        { "String::Replace String", { "", "ScriptCanvas_StringFunctions_ReplaceString" } }
    };

    NodeConfiguration ReplacementUtils::GetReplacementMethodNode(const char* className, const char* methodName)
    {
        NodeConfiguration configuration{};
        AZStd::string oldMethodName = AZStd::string::format("%s::%s", className, methodName);
        if (m_replacementMethods.find(oldMethodName.c_str()) != m_replacementMethods.end())
        {
            configuration.m_type = AZ::Uuid("{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}");
            configuration.m_className = std::get<0>(m_replacementMethods[oldMethodName.c_str()]).c_str();
            configuration.m_methodName = std::get<1>(m_replacementMethods[oldMethodName.c_str()]).c_str();
        }
        return configuration;
    }
} // namespace ScriptCanvas
