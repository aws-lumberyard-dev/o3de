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

#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/set.h>

namespace TestHarness
{
    class TestCaseInfo
        : public AZ::ComponentBus
    {
    public:
        virtual AZ::EntityId GetEntityId() = 0;
        virtual AZStd::string GetTestCaseName() = 0;
        virtual bool IsEnabled() = 0;
        virtual AZStd::set<AZStd::string> GetUserDefinedTags() = 0;
    };

    using TestCaseInfoBus = AZ::EBus<TestCaseInfo>;

    void ReflectTestCaseInfoBus(AZ::ReflectContext* context);

} // namespace TestHarness
