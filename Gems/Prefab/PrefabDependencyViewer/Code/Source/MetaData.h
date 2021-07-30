/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace PrefabDependencyViewer::Utils
{
    using TemplateId = AzToolsFramework::Prefab::TemplateId;

    /**
     * MetaData class is a generic struct that stores debugging
     * information about the Prefabs and Assets dependencies.
     */
    struct MetaData
    {
    public:
        AZ_CLASS_ALLOCATOR(MetaData, AZ::SystemAllocator, 0);

        MetaData() = default;
        virtual AZStd::string_view GetDisplayName() = 0;
    };

    struct PrefabMetaData : public MetaData
    {
    public:
        PrefabMetaData(TemplateId tid, AZStd::string source)
            : MetaData()
            , m_tid(tid)
            , m_source(AZStd::move(source))
        {
        }

        TemplateId GetTemplateId()
        {
            return m_tid;
        }

        AZStd::string_view GetSource()
        {
            return m_source;
        }

        AZStd::string_view GetDisplayName() override {
            return GetSource();
        }

        private:
        TemplateId m_tid;
        AZStd::string m_source;
    };

    struct AssetMetaData : public MetaData
    {
    public:
        AssetMetaData(AZStd::string assetDescription)
            : MetaData()
            , m_assetDescription(assetDescription)
        {
        }

        AZStd::string_view GetDisplayName() override
        {
            return m_assetDescription;
        }

    private:
        AZStd::string m_assetDescription;
    };
} // namespace PrefabDependencyViewer::Utils
