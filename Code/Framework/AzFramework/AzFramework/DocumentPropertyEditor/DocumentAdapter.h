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
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::DocumentPropertyEditor
{
    class DocumentAdapter;
    class RoutingAdapter;
    using DocumentAdapterPtr = AZStd::shared_ptr<DocumentAdapter>;
    using ConstDocumentAdapterPtr = AZStd::shared_ptr<const DocumentAdapter>;

    class DocumentAdapter
    {
    public:
        using ResetEvent = Event<>;
        using ChangedEvent = Event<const Dom::Patch&>;

        virtual ~DocumentAdapter() = default;
        virtual Dom::Value GetContents() const = 0;
        virtual Dom::PatchOutcome RequestContentChange(const Dom::Patch& patch) = 0;

        void ConnectResetHandler(ResetEvent::Handler& handler);
        void ConnectChangedHandler(ChangedEvent::Handler& handler);

        virtual bool SupportsRouting() const;
        virtual RoutingAdapter* GetRoutingAdapter();

    protected:
        void ResetDocument();
        void NotifyContentsChanged(const Dom::Patch& patch);

    private:
        ResetEvent m_resetEvent;
        ChangedEvent m_changedEvent;
    };
} // namespace AZ::DocumentPropertyEditor
