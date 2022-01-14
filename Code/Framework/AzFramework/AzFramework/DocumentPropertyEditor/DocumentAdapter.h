/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValue.h>
#include <AzCore/DOM/DomPatch.h>
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

        void ConnectResetHandler(ResetEvent::Handler& handler);
        void ConnectChangedHandler(ChangedEvent::Handler& handler);

    protected:
        void ResetDocument();
        void ApplyPatchToContents(const AZ::Dom::Patch& patch);

    private:
        ResetEvent m_resetEvent;
        ChangedEvent m_changedEvent;
    };
}
