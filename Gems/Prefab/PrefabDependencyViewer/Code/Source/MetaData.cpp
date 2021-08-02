/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MetaData.h>

namespace PrefabDependencyViewer::Utils
{
    PrefabMetaData::PrefabMetaData(TemplateId tid, AZStd::string source)
        : MetaData()
        , m_tid(tid)
        , m_source(AZStd::move(source))
    {
    }

    TemplateId PrefabMetaData::GetTemplateId()
    {
        return m_tid;
    }

    AZStd::string_view PrefabMetaData::GetSource()
    {
        return m_source;
    }

    AZStd::string_view PrefabMetaData::GetDisplayName()
    {
        return GetSource();
    }

    AssetMetaData::AssetMetaData(AZStd::string source)
        : MetaData()
        , m_source(AZStd::move(source))
    {
    }

    AZStd::string_view AssetMetaData::GetDisplayName()
    {
        return m_source;
    }
}
