/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/EBus/Event.h>

namespace AZ::DocumentPropertyEditor
{
    class DocumentAdapter
    {
    public:
        using ResetEvent = AZ::Event<>;
        using ChangedEvent = AZ::Event<const AZ::Dom::Patch&>;

        virtual ~DocumentAdapter() = default;
        virtual Dom::Value GetContents() const = 0;

        using PatchOutcome = AZ::Outcome<void, AZStd::fixed_string<1024>>;
        virtual void ApplyPatchFromView(const AZ::Dom::Patch& patch) = 0;

        void ConnectResetHandler(ResetEvent::Handler& handler);
        void ConnectChangedHandler(ChangedEvent::Handler& handler);

        static Dom::Value CreateRow();
        static Dom::Value CreateLabel(AZStd::string_view text, bool copy = true);
        static Dom::Value CreatePropertyEditor(AZ::Name editorType);

    protected:
        void ResetDocument();
        virtual void SendPatchToView(const AZ::Dom::Patch& patch) = 0;

    private:
        ResetEvent m_resetEvent;
        ChangedEvent m_changedEvent;
    };
} // namespace AZ::DocumentPropertyEditor
