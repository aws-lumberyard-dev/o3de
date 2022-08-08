/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <Metadata/MetadataManager.h>

#include "assetUtils.h"

namespace AssetProcessor
{
    struct IUuidRequests
    {
        AZ_RTTI(IUuidRequests, "{4EA7E0F6-CB4E-4F9C-ADBC-807676D51772}");

        virtual ~IUuidRequests() = default;

        virtual AZ::Uuid Get(AZ::IO::PathView file) = 0;
        virtual AZ::Uuid GetLegacy(AZ::IO::PathView file) = 0;
    };

    class UuidManager :
        public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static constexpr const char* UuidKey = "UUID";

        static void Reflect(AZ::ReflectContext* context)
        {
            CachedUuid::Reflect(context);
        }

        AZ::Uuid Get(AZ::IO::PathView file) override
        {
            auto entry = GetOrCreateUuidEntry(file);

            return entry.m_uuid;
        }

        AZ::Uuid GetLegacy(AZ::IO::PathView file) override
        {
            auto entry = GetOrCreateUuidEntry(file);

            return entry.m_legacyUuid;
        }

    private:
        struct CachedUuid
        {
            AZ_TYPE_INFO(CachedUuid, "{FAD60D80-9B1D-421D-A4CA-DD2CA2EA80BB}");

            static void Reflect(AZ::ReflectContext* context)
            {
                if(auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<CachedUuid>()
                        ->Version(0)
                        ->Field("uuid", &CachedUuid::m_uuid)
                        ->Field("legacyUuid", &CachedUuid::m_legacyUuid);
                }
            }

            AZ::Uuid m_uuid;
            AZ::Uuid m_legacyUuid;
        };

        AZStd::string GetCanonicalPathKey(AZ::IO::PathView file)
        {
            // TODO: Verify if this needs to be lowercased or not

            return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
        }

        CachedUuid GetOrCreateUuidEntry(AZ::IO::PathView file)
        {
            AZStd::scoped_lock scopeLock(m_uuidMutex);

            auto key = GetCanonicalPathKey(file);
            auto itr = m_uuids.find(key);

            if (itr != m_uuids.end())
            {
                return itr->second;
            }

            auto value = GetMetadataManager()->Get(file, UuidKey);

            if (!value.empty())
            {
                if (value.type() == azrtti_typeid<CachedUuid>())
                {
                    m_uuids[key] = AZStd::any_cast<CachedUuid>(value);

                    return m_uuids[key];
                }

                AZ_Error(
                    "UuidManager",
                    false,
                    "Metadata file has key %s but type is %s instead of %s",
                    key.c_str(),
                    value.type().ToFixedString().c_str(),
                    azrtti_typeid<CachedUuid>().ToFixedString().c_str());

                return {};
            }

            CachedUuid newUuid;

            newUuid.m_uuid = CreateUuid();
            newUuid.m_legacyUuid = CreateLegacyUuid(file);

            GetMetadataManager()->Set(file, UuidKey, AZStd::any(newUuid));

            return newUuid;
        }

        AzToolsFramework::IMetadataRequests* GetMetadataManager()
        {
            if(!m_metadataManager)
            {
                m_metadataManager = AZ::Interface<AzToolsFramework::IMetadataRequests>::Get();
            }

            return m_metadataManager;
        }

        AZ::Uuid CreateUuid()
        {
            return AZ::Uuid::CreateRandom();
        }

        AZ::Uuid CreateLegacyUuid(AZ::IO::PathView file)
        {
            return AssetUtilities::CreateSafeSourceUUIDFromName(file.FixedMaxPathString().c_str());
        }

        AZStd::recursive_mutex m_uuidMutex;
        AZStd::unordered_map<AZStd::string, CachedUuid> m_uuids;

        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
}
