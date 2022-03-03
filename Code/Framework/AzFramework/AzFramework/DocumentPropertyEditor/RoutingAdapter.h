/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomPrefixTree.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class RoutingAdapter : public DocumentAdapter
    {
    public:
        Dom::PatchOutcome RequestContentChange(const Dom::Patch& patch) override;

        void ResetRoutes();
        void AddRoute(const Dom::Path& route, DocumentAdapterPtr adapter);
        void RemoveRoute(const Dom::Path& route);

    protected:
        virtual Dom::PatchOutcome RequestRootContentChange(const Dom::Patch& patch);

    private:
        Dom::Path MapPathToRoute(const Dom::Path& path, const Dom::Path& route);
        Dom::Path MapPathFromRoute(const Dom::Path& path, const Dom::Path& route);

        Dom::Patch MapPatchFromRoute(const Dom::Patch& patch, const Dom::Path& route);
        void MapPatchToRoute(Dom::Patch& patch, const Dom::Path& route);

        struct RouteEntry
        {
            Dom::Path m_path;
            DocumentAdapterPtr m_adapter;
            ResetEvent::Handler m_onAdapterReset;
            ChangedEvent::Handler m_onAdapterChanged;
        };

        Dom::DomPrefixTree<AZStd::unique_ptr<RouteEntry>> m_routes;
    };
} // namespace AZ::DocumentPropertyEditor
