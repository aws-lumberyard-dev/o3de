/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    Dom::PatchOutcome RoutingAdapter::RequestContentChange(const Dom::Patch& patch)
    {
        // Apply each individual patch operation as their own patch, as they may be routed to different adapters
        Dom::Patch tempPatch;
        tempPatch.PushBack({});
        Dom::PatchOutcome result;

        for (const Dom::PatchOperation& op : patch)
        {
            tempPatch[0] = op;
            AZStd::unique_ptr<RouteEntry>* route = m_routes.ValueAtPath(op.GetDestinationPath(), Dom::PrefixTreeMatch::PathAndSubpaths);
            if (route != nullptr)
            {
                MapPatchToRoute(tempPatch, (*route)->m_path);
                Dom::CombinePatchOutcomes(result, (*route)->m_adapter->RequestContentChange(tempPatch));
            }
            else
            {
                Dom::CombinePatchOutcomes(result, RequestRootContentChange(tempPatch));
            }
        }

        return result;
    }

    void RoutingAdapter::ResetRoutes()
    {
        m_routes.Clear();
    }

    void RoutingAdapter::AddRoute(const Dom::Path& route, DocumentAdapterPtr adapter)
    {
        AZStd::unique_ptr<RouteEntry> entry = AZStd::make_unique<RouteEntry>();
        entry->m_path = route;
        entry->m_adapter = adapter;
        entry->m_onAdapterChanged = ChangedEvent::Handler(
            [this, entryPtr = entry.get()](const Dom::Patch& patch)
            {
                // On patch, map it to the actual path inthis adapter and notify the view
                NotifyContentsChanged(MapPatchFromRoute(patch, entryPtr->m_path));
            });
        entry->m_onAdapterReset = ResetEvent::Handler(
            [this, entryPtr = entry.get()]()
            {
                // On reset, create a replace operation that updates the entire contents of this route
                Dom::Patch patch;
                patch.PushBack(Dom::PatchOperation::ReplaceOperation(entryPtr->m_path, entryPtr->m_adapter->GetContents()));
                NotifyContentsChanged(patch);
            });
        adapter->ConnectChangedHandler(entry->m_onAdapterChanged);
        adapter->ConnectResetHandler(entry->m_onAdapterReset);
        m_routes.SetValue(route, AZStd::move(entry));
    }

    void RoutingAdapter::RemoveRoute(const Dom::Path& route)
    {
        m_routes.EraseValue(route);
    }

    bool RoutingAdapter::SupportsRouting() const
    {
        return true;
    }

    RoutingAdapter* RoutingAdapter::GetRoutingAdapter()
    {
        return this;
    }

    Dom::PatchOutcome RoutingAdapter::RequestRootContentChange([[maybe_unused]] const Dom::Patch& patch)
    {
        return AZ::Failure<AZStd::string>(
            "Attempted to call RequestRootContentChange on a RoutingAdapter that does not have it implemented.\nCheck to see if there "
            "should be an implementation, or if this should have been caught by a route.");
    }

    Dom::Path RoutingAdapter::MapPathToRoute(const Dom::Path& path, const Dom::Path& route)
    {
        return route / path;
    }

    Dom::Path RoutingAdapter::MapPathFromRoute(const Dom::Path& path, const Dom::Path& route)
    {
        Dom::Path subpath;
        for (size_t i = route.size(); i < path.size(); ++i)
        {
            subpath.Push(path[i]);
        }
        return subpath;
    }

    Dom::Patch RoutingAdapter::MapPatchFromRoute(const Dom::Patch& patch, const Dom::Path& route)
    {
        Dom::Patch routedPatch = patch;
        for (Dom::PatchOperation& entry : routedPatch)
        {
            entry.SetDestinationPath(MapPathFromRoute(entry.GetDestinationPath(), route));
            const Dom::PatchOperation::Type entryType = entry.GetType();
            if (entryType == Dom::PatchOperation::Type::Move || entryType == Dom::PatchOperation::Type::Copy)
            {
                entry.SetSourcePath(MapPathFromRoute(entry.GetSourcePath(), route));
            }
        }
        return routedPatch;
    }

    void RoutingAdapter::MapPatchToRoute(Dom::Patch& patch, const Dom::Path& route)
    {
        for (Dom::PatchOperation& entry : patch)
        {
            entry.SetDestinationPath(MapPathToRoute(entry.GetDestinationPath(), route));
            const Dom::PatchOperation::Type entryType = entry.GetType();
            if (entryType == Dom::PatchOperation::Type::Move || entryType == Dom::PatchOperation::Type::Copy)
            {
                entry.SetSourcePath(MapPathToRoute(entry.GetSourcePath(), route));
            }
        }
    }
} // namespace AZ::DocumentPropertyEditor
