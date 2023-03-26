/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestSystem.h>

namespace TestImpact
{
    //!
    using NativeShardedInstrumentedTestSystemNotifications = NativeShardedTestSystemNotifications<NativeInstrumentedTestRunner>;

    //!
    class NativeShardedInstrumentedTestRunner
        : public NativeShardedTestSystem<NativeInstrumentedTestRunner>
    {
    public:
        //!
        using NativeShardedTestSystem<NativeInstrumentedTestRunner>::NativeShardedTestSystem;

    private:
        // NativeShardedTestSystem overrides ...
        [[nodiscard]] virtual typename NativeInstrumentedTestRunner::ResultType ConsolidateSubJobs(
            const typename NativeInstrumentedTestRunner::ResultType& result,
            const ShardToParentShardedJobMap& shardToParentShardedJobMap,
            const CompletedShardMap& completedShardMap) override;
    };
} // namespace TestImpact
