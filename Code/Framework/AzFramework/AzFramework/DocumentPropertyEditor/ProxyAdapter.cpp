/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/ProxyAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    ProxyAdapter::ProxyAdapter(DocumentAdapterPtr adapterToProxy)
    {
        m_onProxiedAdapterReset = ResetEvent::Handler([this]()
            {
                OnProxiedAdapterReset();
            });
        m_onProxiedAdapterChanged = ChangedEvent::Handler([this](const Dom::Patch& patch)
            {
                OnProxiedAdapterChanged(patch);
            });
        SetProxiedAdapter(adapterToProxy);
    }

    Dom::Value ProxyAdapter::GetContents() const
    {
        return m_proxiedAdapter != nullptr ? m_proxiedAdapter->GetContents() : Dom::Value();
    }

    Dom::PatchOutcome ProxyAdapter::RequestContentChange(const Dom::Patch& patch)
    {
        return m_proxiedAdapter->RequestContentChange(patch);
    }

    DocumentAdapterPtr ProxyAdapter::GetProxiedAdapter()
    {
        return m_proxiedAdapter;
    }

    ConstDocumentAdapterPtr ProxyAdapter::GetProxiedAdapter() const
    {
        return m_proxiedAdapter;
    }

    void ProxyAdapter::SetProxiedAdapter(DocumentAdapterPtr adapterToProxy)
    {
        if (m_proxiedAdapter)
        {
            m_onProxiedAdapterReset.Disconnect();
            m_onProxiedAdapterChanged.Disconnect();
        }

        m_proxiedAdapter = adapterToProxy;
        m_proxiedAdapter->ConnectResetHandler(m_onProxiedAdapterReset);
        m_proxiedAdapter->ConnectChangedHandler(m_onProxiedAdapterChanged);
    }

    void ProxyAdapter::OnProxiedAdapterReset()
    {
        ResetDocument();
    }

    void ProxyAdapter::OnProxiedAdapterChanged(const Dom::Patch& patch)
    {
        NotifyContentsChanged(patch);
    }
}
