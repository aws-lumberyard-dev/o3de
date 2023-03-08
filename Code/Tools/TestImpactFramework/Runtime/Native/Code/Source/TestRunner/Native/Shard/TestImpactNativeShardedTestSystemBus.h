/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/JobRunner/TestImpactProcessJobMeta.h>
#include <Process/JobRunner/TestImpactProcessJobRunner.h>

#include <AzCore/EBus/EBus.h>

#pragma once

namespace TestImpact
{
    //! Bus for native sharded test system notifications.
    template<typename TestRunnerType>
    class NativeShardedTestSystemNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Callback for sharded job completion/failure.
        //! @param jobInfo The job information associated with this job.
        //! @param meta The meta-data about the job run.
        //! @param std The standard output and standard error of the process running the job.
        virtual ProcessCallbackResult OnShardedJobComplete(
            [[maybe_unused]] const typename TestRunnerType::Job::Info& jobInfo,
            [[maybe_unused]] const JobMeta& meta,
            [[maybe_unused]] const StdContent& std)
        {
            return ProcessCallbackResult::Continue;
        }

        //! Callback for sharded sub-job completion/failure.
        //! @param subJobCount The number of sub-jobs that make up this job.
        //! @param jobId The id of the sharded job.
        //! @param subJobInfo The job information associated with this sharded sub-job.
        //! @param subJobMeta The meta-data about the sharded sub-job run.
        //! @param subJobStd The standard output and standard error of the process running the sharded sub-job.
        virtual ProcessCallbackResult OnShardedSubJobComplete(
            [[maybe_unused]] typename TestRunnerType::JobInfo::Id jobId,
            [[maybe_unused]] size_t subJobCount,
            [[maybe_unused]] const typename TestRunnerType::Job::Info& subJobInfo,
            [[maybe_unused]] const JobMeta& subJobMeta,
            [[maybe_unused]] const StdContent& subJobStd)
        {
            return ProcessCallbackResult::Continue;
        }
    };

    template<typename TestRunnerType>
    using NativeShardedTestSystemNotificationsBus = AZ::EBus<NativeShardedTestSystemNotifications<TestRunnerType>>;
} // namespace TestImpact
