/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
<<<<<<<< HEAD:Code/Tools/TestImpactFramework/Runtime/Common/Code/Tests/TestProcess/Code/Source/TestImpactTestProcessLargeText.h

namespace TestImpact
{
    // Large text blob in the form of a string literal so the app does not require the Az FileIO and Application environment
    extern const char* const LongText;
========

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonTestRunnerBase.h>

namespace TestImpact
{
    class PythonTestRunner
        : public PythonTestRunnerBase
    {
    public:
        using PythonTestRunnerBase::PythonTestRunnerBase;
    };
>>>>>>>> MergeHelper:Code/Tools/TestImpactFramework/Runtime/Python/Code/Source/TestRunner/Python/TestImpactPythonTestRunner.h
} // namespace TestImpact
