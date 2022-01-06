/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Benchmark
{
    using DomPatchBenchmark = Tests::DomBenchmarkFixture;

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplySimpleReplace)(benchmark::State& state)
    {
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatchApplySimpleReplace)
}
