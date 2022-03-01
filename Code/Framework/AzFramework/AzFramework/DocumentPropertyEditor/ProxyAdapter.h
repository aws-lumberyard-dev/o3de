/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class ProxyAdapter : public DocumentAdapter
    {
    public:
        ProxyAdapter(DocumentAdapterPtr adapterToProxy);

        Dom::Value GetContents() const override;
        Dom::PatchOutcome RequestContentChange(const Dom::Patch& patch) override;

        DocumentAdapterPtr GetProxiedAdapter();
        ConstDocumentAdapterPtr GetProxiedAdapter() const;
        void SetProxiedAdapter(DocumentAdapterPtr adapterToProxy);

    protected:
        virtual void OnProxiedAdapterReset();
        virtual void OnProxiedAdapterChanged(const Dom::Patch& patch);

    private:
        DocumentAdapterPtr m_proxiedAdapter;
        ResetEvent::Handler m_onProxiedAdapterReset;
        ChangedEvent::Handler m_onProxiedAdapterChanged;
    };
}
