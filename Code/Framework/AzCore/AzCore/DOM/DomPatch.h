/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPath.h>
#include <AzCore/DOM/DomValue.h>

namespace AZ::Dom
{
    class PatchOperation final
    {
    public:
        using PatchOutcome = AZ::Outcome<void, AZStd::string>;

        enum class Type
        {
            Add,
            Remove,
            Replace,
            Copy,
            Move,
            Test
        };

        PatchOperation() = default;
        PatchOperation(const PatchOperation&) = default;
        PatchOperation(PatchOperation&&) = default;
        explicit PatchOperation(Path path, Type type, Value value);

        PatchOperation& operator=(const PatchOperation&) = default;
        PatchOperation& operator=(PatchOperation&&) = default;

        Type GetType() const;
        void SetType(Type type);

        const Path& GetDestinationPath() const;
        void SetDestinationPath(Path path);

        const Value& GetValue() const;
        void SetValue(Value value);

        const Path& GetSourcePath() const;
        void SetSourcePath(Path path);

        AZ::Outcome<Value, AZStd::string> Apply(Value rootElement) const;
        PatchOutcome ApplyInPlace(Value& rootElement) const;

        Value GetDomRepresentation() const;
        static AZ::Outcome<PatchOperation, AZStd::string> CreateFromDomRepresentation(Value domValue);

        PatchOperation GetInverse() const;

    private:
        Path m_domPath;
        Type m_operation;
        AZStd::variant<AZStd::monostate, Value, Path> m_value;
    };

    class Patch;

    struct PatchingState
    {
        const Patch& m_patch;
        PatchOperation::PatchOutcome m_outcome;
        Value& m_currentState;
        bool m_shouldContinue = true;
    };

    namespace PatchStrategy
    {
        void IgnorePatchFailureAndContinue(PatchingState& state);
        void HaltOnFailure(PatchingState& state);
    } // namespace PatchStrategy

    class Patch final
    {
    public:
        using StrategyFunctor = AZStd::function<void(PatchingState&)>;

        AZ::Outcome<Value, AZStd::string> Apply(Value rootElement, StrategyFunctor strategy = PatchStrategy::HaltOnFailure) const;
        AZ::Outcome<void, AZStd::string> ApplyInPlace(Value& rootElement, StrategyFunctor strategy = PatchStrategy::HaltOnFailure) const;

        Value GetDomRepresentation() const;
        static AZ::Outcome<Patch, AZStd::string> CreateFromDomRepresentation(Value domValue);

    private:
        AZStd::vector<PatchOperation> m_operations;
    };

    //! A set of patches for applying a change and doing the inverse operation (i.e. undoing it).
    struct PatchInfo
    {
        Patch m_forwardPatches;
        Patch m_inversePatches;
    };

    //! Generates a set of patches such that m_forwardPatches.Apply(beforeState) shall
    //! produce a document equivalent to afterState, and a subsequent m_inversePatches.Apply(beforeState)
    //! shall produce the original document.
    //! This patch generation strategy does a hierarchical comparison and is not
    //! guaranteed to create the minimal set of patches required to transform
    //! between the two states.
    PatchInfo GenerateHierarchicalDeltaPatch(const Value& beforeState, const Value& afterState);
} // namespace AZ::Dom
