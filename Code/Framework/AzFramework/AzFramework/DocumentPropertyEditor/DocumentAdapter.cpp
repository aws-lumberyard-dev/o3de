/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    Dom::Value DocumentAdapter::CreateRow()
    {
        return Dom::Value::CreateNode(AZ_NAME_LITERAL("Row"));
    }

    Dom::Value DocumentAdapter::CreateLabel(AZStd::string_view text, bool copy)
    {
        Dom::Value labelNode = Dom::Value::CreateNode(AZ_NAME_LITERAL("Label"));
        Dom::Value labelText(text, copy);
        labelNode.SetNodeValue(labelText);
        return labelNode;
    }

    Dom::Value DocumentAdapter::CreatePropertyEditor(AZ::Name editorType)
    {
        return Dom::Value::CreateNode(AZStd::move(editorType));
    }

    void DocumentAdapter::ConnectResetHandler(ResetEvent::Handler& handler)
    {
        handler.Connect(m_resetEvent);
    }

    void DocumentAdapter::ConnectChangedHandler(ChangedEvent::Handler& handler)
    {
        handler.Connect(m_changedEvent);
    }

    void DocumentAdapter::ResetDocument()
    {
        m_resetEvent.Signal();
    }

    void DocumentAdapter::SendPatchToView(const AZ::Dom::Patch& patch)
    {
        m_changedEvent.Signal(patch);
    }
} // namespace AZ::DocumentPropertyEditor
