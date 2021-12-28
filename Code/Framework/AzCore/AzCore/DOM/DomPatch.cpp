/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>

namespace AZ::Dom
{
    PatchOperation::PatchOperation(Path destinationPath, Type type, Value value)
        : m_domPath(destinationPath)
        , m_type(type)
        , m_value(value)
    {
    }

    PatchOperation::PatchOperation(Path destinationPath, Type type, Path sourcePath)
        : m_domPath(destinationPath)
        , m_type(type)
        , m_value(sourcePath)
    {
    }

    PatchOperation::PatchOperation(Path destinationPath, Type type)
        : m_domPath(destinationPath)
        , m_type(type)
    {
    }

    PatchOperation::Type PatchOperation::GetType() const
    {
        return m_type;
    }

    void PatchOperation::SetType(Type type)
    {
        m_type = type;
    }

    const Path& PatchOperation::GetDestinationPath() const
    {
        return m_domPath;
    }

    void PatchOperation::SetDestinationPath(Path path)
    {
        m_domPath = path;
    }

    const Value& PatchOperation::GetValue() const
    {
        return AZStd::get<Value>(m_value);
    }

    void PatchOperation::SetValue(Value value)
    {
        m_value = AZStd::move(value);
    }

    const Path& PatchOperation::GetSourcePath() const
    {
        return AZStd::get<Path>(m_value);
    }

    void PatchOperation::SetSourcePath(Path path)
    {
        m_value = AZStd::move(path);
    }

    AZ::Outcome<Value, AZStd::string> PatchOperation::Apply(Value rootElement) const
    {
        PatchOutcome outcome = ApplyInPlace(rootElement);
        if (!outcome.IsSuccess())
        {
            return AZ::Failure(outcome.TakeError());
        }
        return AZ::Success(AZStd::move(rootElement));
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyInPlace(Value& rootElement) const
    {
        switch (m_type)
        {
        case Type::Add:
            return ApplyAdd(rootElement);
        case Type::Remove:
            return ApplyRemove(rootElement);
        case Type::Replace:
            return ApplyReplace(rootElement);
        case Type::Copy:
            return ApplyCopy(rootElement);
        case Type::Move:
            return ApplyMove(rootElement);
        case Type::Test:
            return ApplyTest(rootElement);
        }
        return AZ::Failure<AZStd::string>("Unsupported DOM patch operation specified");
    }

    Value PatchOperation::GetDomRepresentation() const
    {
        Value serializedPatch(Dom::Type::Object);
        switch (m_type)
        {
        case Type::Add:
            serializedPatch["op"] = "add";
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        case Type::Remove:
            serializedPatch["op"] = "remove";
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Replace:
            serializedPatch["op"] = "replace";
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        case Type::Copy:
            serializedPatch["op"] = "copy";
            serializedPatch["from"].CopyFromString(GetSourcePath().ToString());
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Move:
            serializedPatch["op"] = "move";
            serializedPatch["from"].CopyFromString(GetSourcePath().ToString());
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            break;
        case Type::Test:
            serializedPatch["op"] = "test";
            serializedPatch["path"].CopyFromString(GetDestinationPath().ToString());
            serializedPatch["value"] = GetValue();
            break;
        default:
            AZ_Assert(false, "PatchOperation::GetDomRepresentation: invalid patch type specified");
        }
        return serializedPatch;
    }

    AZ::Outcome<PatchOperation, AZStd::string> PatchOperation::CreateFromDomRepresentation(Value domValue)
    {
        if (!domValue.IsObject())
        {
            return AZ::Failure<AZStd::string>("PatchOperation failed to load: PatchOperation must be specified as an Object");
        }

        auto loadField = [&](const char* field, AZStd::optional<Dom::Type> type = {}) -> AZ::Outcome<Value, AZStd::string>
        {
            auto it = domValue.FindMember(field);
            if (it == domValue.MemberEnd())
            {
                return AZ::Failure(AZStd::string::format("PatchOperation failed to load: no \"%s\" specified", field));
            }

            if (type.has_value() && it->second.GetType() != type)
            {
                return AZ::Failure(AZStd::string::format("PatchOperation failed to load: \"%s\" is invalid", field));
            }

            return AZ::Success(it->second);
        };

        auto opLoad = loadField("op", Dom::Type::String);
        if (!opLoad.IsSuccess())
        {
            return AZ::Failure(opLoad.TakeError());
        }
        AZStd::string_view op = opLoad.GetValue().GetString();
        if (op == "add")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(Patch::AddOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else if (op == "remove")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }

            return AZ::Success(Patch::RemoveOperation(Path(pathLoad.GetValue().GetString())));
        }
        else if (op == "replace")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(Patch::ReplaceOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else if (op == "copy")
        {
            auto destLoad = loadField("path", Dom::Type::String);
            if (!destLoad.IsSuccess())
            {
                return AZ::Failure(destLoad.TakeError());
            }
            auto sourceLoad = loadField("from", Dom::Type::String);
            if (!sourceLoad.IsSuccess())
            {
                return AZ::Failure(sourceLoad.TakeError());
            }

            return AZ::Success(Patch::CopyOperation(Path(destLoad.GetValue().GetString()), Path(sourceLoad.GetValue().GetString())));
        }
        else if (op == "move")
        {
            auto destLoad = loadField("path", Dom::Type::String);
            if (!destLoad.IsSuccess())
            {
                return AZ::Failure(destLoad.TakeError());
            }
            auto sourceLoad = loadField("from", Dom::Type::String);
            if (!sourceLoad.IsSuccess())
            {
                return AZ::Failure(sourceLoad.TakeError());
            }

            return AZ::Success(Patch::MoveOperation(Path(destLoad.GetValue().GetString()), Path(sourceLoad.GetValue().GetString())));
        }
        else if (op == "test")
        {
            auto pathLoad = loadField("path", Dom::Type::String);
            if (!pathLoad.IsSuccess())
            {
                return AZ::Failure(pathLoad.TakeError());
            }
            auto valueLoad = loadField("value");
            if (!valueLoad.IsSuccess())
            {
                return AZ::Failure(valueLoad.TakeError());
            }

            return AZ::Success(Patch::TestOperation(Path(pathLoad.GetValue().GetString()), valueLoad.TakeValue()));
        }
        else
        {
            return AZ::Failure<AZStd::string>("PatchOperation failed to create DOM representation: invalid \"op\" specified");
        }
    }

    AZ::Outcome<PatchOperation, AZStd::string> PatchOperation::GetInverse(Value stateBeforeApplication) const
    {
        switch (m_type)
        {
        case Type::Add:
            {
                // Add -> Replace (if value already existed) otherwise
                // Add -> Remove
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue != nullptr)
                {
                    return AZ::Success(Patch::ReplaceOperation(m_domPath, *existingValue));
                }
                return AZ::Success(Patch::RemoveOperation(m_domPath));
            }
        case Type::Remove:
            {
                // Remove -> Add
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    return AZ::Failure(
                        AZStd::string::format("Unable to invert DOM remove patch, source path not found: %s", m_domPath.ToString().data()));
                }
                return AZ::Success(Patch::AddOperation(m_domPath, *existingValue));
            }
        case Type::Replace:
            {
                // Replace -> Replace (with old value)
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Unable to invert DOM replace patch, source path not found: %s", m_domPath.ToString().data()));
                }
                return AZ::Success(Patch::ReplaceOperation(m_domPath, *existingValue));
            }
        case Type::Copy:
            {
                // Copy -> Replace (with old value)
                const Value* existingValue = stateBeforeApplication.FindChild(m_domPath);
                if (existingValue == nullptr)
                {
                    return AZ::Failure(
                        AZStd::string::format("Unable to invert DOM copy patch, source path not found: %s", m_domPath.ToString().data()));
                }
                return AZ::Success(Patch::ReplaceOperation(m_domPath, *existingValue));
            }
        case Type::Move:
            {
                // Move -> Replace, using the common ancestor of the two paths as the replacement
                // This is not a minimal inverse, which would be two replace operations at each path
                const Path& destPath = m_domPath;
                const Path& sourcePath = GetSourcePath();

                Path commonAncestor;
                for (size_t i = 0; i < destPath.Size() && i < sourcePath.Size(); ++i)
                {
                    if (destPath[i] != sourcePath[i])
                    {
                        break;
                    }

                    commonAncestor.Push(destPath[i]);
                }

                const Value* existingValue = stateBeforeApplication.FindChild(commonAncestor);
                if (existingValue == nullptr)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Unable to invert DOM move patch, common ancestor path not found: %s", commonAncestor.ToString().data()));
                }
                return AZ::Success(Patch::ReplaceOperation(commonAncestor, *existingValue));
            }
        case Type::Test:
            {
                // Test -> Test (no change)
                // When inverting a sequence of patches, applying them in reverse order should allow the test to continue to succeed
                return AZ::Success(*this);
            }
        }
        return AZ::Failure<AZStd::string>("Unable to invert DOM patch, unknown type specified");
    }

    AZ::Outcome<PatchOperation::PathContext, AZStd::string> PatchOperation::LookupPath(Value& rootElement, const Path& path, bool checkExistence)
    {
        Path target = path;
        if (target.Size() == 0)
        {
            return AZ::Failure<AZStd::string>("Empty path specified");
        }
        PathEntry destinationIndex = target[target.Size() - 1];
        target.Pop();

        Value* targetValue = rootElement.FindMutableChild(target);
        if (targetValue == nullptr)
        {
            return AZ::Failure(
                AZStd::string::format("Path not found (%s)", target.ToString().data()));
        }

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            if (!targetValue->IsArray() && !targetValue->IsNode())
            {
                return AZ::Failure<AZStd::string>(
                    "Array index specified for a value that is not an array or node"
                );
            }

            if (destinationIndex.IsIndex() && destinationIndex.GetIndex() >= targetValue->Size())
            {
                return AZ::Failure<AZStd::string>("Array index out bounds");
            }
        }
        else
        {
            if (!targetValue->IsObject() && !targetValue->IsNode())
            {
                return AZ::Failure<AZStd::string>("Key specified for a value that is not an object or node");
            }

            if (checkExistence)
            {
                if (auto it = targetValue->FindMember(destinationIndex.GetKey()); it == targetValue->MemberEnd())
                {
                    return AZ::Failure<AZStd::string>("Key not found in container");
                }
            }
        }

        return AZ::Success<PathContext>({*targetValue, AZStd::move(destinationIndex)});
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyAdd(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, false);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }
        const PathContext& context = pathLookup.GetValue();
        const PathEntry& destinationIndex = context.m_key;
        Value& targetValue = context.m_value;

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            if (destinationIndex.IsEndOfArray())
            {
                targetValue.PushBack(GetValue());
            }
            else
            {
                const size_t index = destinationIndex.GetIndex();
                targetValue[index] = GetValue();
            }
        }
        else
        {
            targetValue[destinationIndex] = GetValue();
        }
        return AZ::Success();
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyRemove(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, true);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }
        const PathContext& context = pathLookup.GetValue();
        const PathEntry& destinationIndex = context.m_key;
        Value& targetValue = context.m_value;

        if (destinationIndex.IsIndex() || destinationIndex.IsEndOfArray())
        {
            size_t index = destinationIndex.IsEndOfArray() ? targetValue.Size() - 1 : destinationIndex.GetIndex();
            targetValue.Erase(targetValue.Begin() + index);
        }
        else
        {
            auto it = targetValue.FindMember(destinationIndex.GetKey());
            targetValue.EraseMember(it);
        }
        return AZ::Success();
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyReplace(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, true);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }

        rootElement[m_domPath] = GetValue();
        return AZ::Success();
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyCopy(Value& rootElement) const
    {
        auto sourceLookup = LookupPath(rootElement, GetSourcePath(), true);
        if (!sourceLookup.IsSuccess())
        {
            return AZ::Failure(sourceLookup.TakeError());
        }

        auto destLookup = LookupPath(rootElement, m_domPath, false);
        if (!destLookup.IsSuccess())
        {
            return AZ::Failure(destLookup.TakeError());
        }

        rootElement[m_domPath] = rootElement[GetSourcePath()];
        return AZ::Success();
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyMove(Value& rootElement) const
    {
        auto sourceLookup = LookupPath(rootElement, GetSourcePath(), true);
        if (!sourceLookup.IsSuccess())
        {
            return AZ::Failure(sourceLookup.TakeError());
        }

        auto destLookup = LookupPath(rootElement, m_domPath, false);
        if (!destLookup.IsSuccess())
        {
            return AZ::Failure(destLookup.TakeError());
        }

        Value valueToMove = rootElement[GetSourcePath()];
        const PathContext& sourceContext = sourceLookup.GetValue();
        if (sourceContext.m_key.IsEndOfArray())
        {
            sourceContext.m_value.PopBack();
        }
        else if (sourceContext.m_key.IsIndex())
        {
            sourceContext.m_value.Erase(sourceContext.m_value.Begin() + sourceContext.m_key.GetIndex());
        }
        else
        {
            sourceContext.m_value.EraseMember(sourceContext.m_key.GetKey());
        }

        rootElement[m_domPath] = AZStd::move(valueToMove);
        return AZ::Success();
    }

    PatchOperation::PatchOutcome PatchOperation::ApplyTest(Value& rootElement) const
    {
        auto pathLookup = LookupPath(rootElement, m_domPath, true);
        if (!pathLookup.IsSuccess())
        {
            return AZ::Failure(pathLookup.TakeError());
        }

        if (!rootElement[m_domPath].DeepCompareIsEqual(GetValue()))
        {
            return AZ::Failure<AZStd::string>("Test failed, values don't match");
        }

        return AZ::Success();
    }
} // namespace AZ::Dom