/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Native/Shard/TestImpactNativeShardedTestSystem.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>

namespace TestImpact
{
    //!
    using NativeShardedRegularTestSystemNotifications = NativeShardedTestSystemNotifications<NativeRegularTestRunner>;

    //!
    class NativeShardedRegularTestRunner
        : public NativeShardedTestSystem<NativeRegularTestRunner>
    {
    public:
        //!
        using NativeShardedTestSystem<NativeRegularTestRunner>::NativeShardedTestSystem;

    private:
        // NativeShardedTestSystem overrides ...
        [[nodiscard]] virtual typename NativeRegularTestRunner::ResultType ConsolidateSubJobs(
            const typename NativeRegularTestRunner::ResultType& result,
            const ShardToParentShardedJobMap& shardToParentShardedJobMap,
            const CompletedShardMap& completedShardMap) override;
    };
} // namespace TestImpact
