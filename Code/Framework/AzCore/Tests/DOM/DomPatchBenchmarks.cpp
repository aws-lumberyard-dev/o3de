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
#include <AzCore/DOM/DomPatch.h>

namespace AZ::Dom::Benchmark
{
    using DomPatchBenchmark = Tests::DomBenchmarkFixture;

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplySimpleReplace)(benchmark::State& state)
    {
        Value before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
        Value after = before;
        after["entries"]["Key0"] = Value("replacement string", true);

        for (auto _ : state)
        {
            auto patchInfo = GenerateHierarchicalDeltaPatch(before, after);
            auto patchResult = patchInfo.m_forwardPatches.Apply(before);
            benchmark::DoNotOptimize(patchResult);
        }
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomPatchBenchmark, AzDomPatchApplySimpleReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplyTopLevelReplace)(benchmark::State& state)
    {
        Value before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
        Value after = Value(Type::Object);
        after["UnrelatedKey"] = Value(42);

        for (auto _ : state)
        {
            auto patchInfo = GenerateHierarchicalDeltaPatch(before, after);
            auto patchResult = patchInfo.m_forwardPatches.Apply(before);
            benchmark::DoNotOptimize(patchResult);
        }
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomPatchBenchmark, AzDomPatchApplyTopLevelReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplyKeyRemove)(benchmark::State& state)
    {
        Value before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
        Value after = before;
        before["entries"].RemoveMember("Key1");

        for (auto _ : state)
        {
            auto patchInfo = GenerateHierarchicalDeltaPatch(before, after);
            auto patchResult = patchInfo.m_forwardPatches.Apply(before);
            benchmark::DoNotOptimize(patchResult);
        }
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomPatchBenchmark, AzDomPatchApplyKeyRemove)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplyArrayAppend)(benchmark::State& state)
    {
        Value before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
        Value after = before;
        before["entries"]["Key2"].ArrayPushBack(Value(0));

        for (auto _ : state)
        {
            auto patchInfo = GenerateHierarchicalDeltaPatch(before, after);
            auto patchResult = patchInfo.m_forwardPatches.Apply(before);
            benchmark::DoNotOptimize(patchResult);
        }
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomPatchBenchmark, AzDomPatchApplyArrayAppend)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatchApplyArrayPrepend)(benchmark::State& state)
    {
        Value before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
        Value after = before;
        auto& arr = before["entries"]["Key2"].GetMutableArray();
        arr.insert(arr.begin(), Value(42));

        for (auto _ : state)
        {
            auto patchInfo = GenerateHierarchicalDeltaPatch(before, after);
            auto patchResult = patchInfo.m_forwardPatches.Apply(before);
            benchmark::DoNotOptimize(patchResult);
        }
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(DomPatchBenchmark, AzDomPatchApplyArrayPrepend)
}
