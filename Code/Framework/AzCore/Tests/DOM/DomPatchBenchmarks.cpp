/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Benchmark
{
    class DomPatchBenchmark : public Tests::DomBenchmarkFixture
    {
    public:
        void TearDownHarness() override
        {
            m_before = {};
            m_after = {};
            Tests::DomBenchmarkFixture::TearDownHarness();
        }

        void SimpleReplace(benchmark::State& state, bool deepCopy)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"]["Key0"] = Value("replacement string", true);

            BenchmarkPatchGenerateAndApply(state);
        }

        void TopLevelReplace(benchmark::State& state)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = Value(Type::Object);
            m_after["UnrelatedKey"] = Value(42);

            BenchmarkPatchGenerateAndApply(state);
        }

        void KeyRemove(benchmark::State& state, bool deepCopy)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"].RemoveMember("Key1");

            BenchmarkPatchGenerateAndApply(state);
        }

        void ArrayAppend(benchmark::State& state, bool deepCopy)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            m_after["entries"]["Key2"].ArrayPushBack(Value(0));

            BenchmarkPatchGenerateAndApply(state);
        }

        void ArrayPrepend(benchmark::State& state, bool deepCopy)
        {
            m_before = GenerateDomBenchmarkPayload(state.range(0), state.range(1));
            m_after = deepCopy ? Utils::DeepCopy(m_before) : m_before;
            auto& arr = m_after["entries"]["Key2"].GetMutableArray();
            arr.insert(arr.begin(), Value(42));

            BenchmarkPatchGenerateAndApply(state);
        }

    private:
        void BenchmarkPatchGenerateAndApply(benchmark::State&state)
        {
            for (auto _ : state)
            {
                auto patchInfo = GenerateHierarchicalDeltaPatch(m_before, m_after);
                auto patchResult = patchInfo.m_forwardPatches.Apply(m_before);
                benchmark::DoNotOptimize(patchResult);
            }

            state.SetItemsProcessed(state.iterations());
        }

        Value m_before;
        Value m_after;
    };

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_SimpleReplace_ShallowCopy)(benchmark::State& state)
    {
        SimpleReplace(state, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_SimpleReplace_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_SimpleReplace_DeepCopy)(benchmark::State& state)
    {
        SimpleReplace(state, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_SimpleReplace_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_TopLevelReplace)(benchmark::State& state)
    {
        TopLevelReplace(state);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_TopLevelReplace)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_KeyRemove_ShallowCopy)(benchmark::State& state)
    {
        KeyRemove(state, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_KeyRemove_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_KeyRemove_DeepCopy)(benchmark::State& state)
    {
        KeyRemove(state, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_KeyRemove_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_ArrayAppend_ShallowCopy)(benchmark::State& state)
    {
        ArrayAppend(state, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_ArrayAppend_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_ArrayAppend_DeepCopy)(benchmark::State& state)
    {
        ArrayAppend(state, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_ArrayAppend_DeepCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_ArrayPrepend_ShallowCopy)(benchmark::State& state)
    {
        ArrayPrepend(state, false);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_ArrayPrepend_ShallowCopy)

    BENCHMARK_DEFINE_F(DomPatchBenchmark, AzDomPatch_ArrayPrepend_DeepCopy)(benchmark::State& state)
    {
        ArrayPrepend(state, true);
    }
    DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(DomPatchBenchmark, AzDomPatch_ArrayPrepend_DeepCopy)
} // namespace AZ::Dom::Benchmark
