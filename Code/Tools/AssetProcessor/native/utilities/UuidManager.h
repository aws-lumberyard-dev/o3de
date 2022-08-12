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
    #pragma optimize("", off)
    struct IUuidRequests
    {
        AZ_RTTI(IUuidRequests, "{4EA7E0F6-CB4E-4F9C-ADBC-807676D51772}");

        virtual ~IUuidRequests() = default;

        virtual AZ::Uuid GetUuidAbsPath(AZ::IO::PathView absolutePath) = 0;
        virtual AZ::Uuid GetUuidRelPathAndScanfolder(AZ::IO::PathView scanFolderPath, AZ::IO::PathView relativePath) = 0;
        virtual AZ::Uuid GetLegacyUuidAbsPath(AZ::IO::PathView absolutePath) = 0;
        virtual AZ::Uuid GetLegacyUuidRelPathAndScanfolder(AZ::IO::PathView scanfolderPath, AZ::IO::PathView relativePath) = 0;
    };

    class SourceIdentifier
    {
    public:
        SourceIdentifier(AZ::IO::PathView scanfolderPath, AZ::IO::PathView relativePath, const PlatformConfiguration* platformConfiguration)
            : m_scanfolderPath(scanfolderPath), m_relativePath(relativePath)
        {
            if(m_scanfolderPath.empty())
            {
                QString absolutePath = platformConfiguration->FindFirstMatchingFile(m_relativePath.c_str());

                if (absolutePath.isEmpty())
                {
                    return;
                }

                //AZ_Assert(!absolutePath.isEmpty(), "Failed to find file %s", m_relativePath.c_str());

                QString databaseSourceName, scanfolderName;

                if(!platformConfiguration->ConvertToRelativePath(absolutePath, databaseSourceName, scanfolderName))
                {
                    AZ_Assert(false, "Failed to convert path %s to a relative path", absolutePath.toUtf8().constData());
                }

                m_absolutePath = absolutePath.toUtf8().constData();
                m_relativePath = databaseSourceName.toUtf8().constData();
                m_scanfolderPath = scanfolderName.toUtf8().constData();
            }
            else
            {
                m_absolutePath = AZ::IO::Path(m_scanfolderPath) / m_relativePath;
            }
        }

        SourceIdentifier(AZ::IO::PathView absolutePath, const PlatformConfiguration* platformConfiguration)
            : m_absolutePath(absolutePath)
        {
            QString databaseSourceName, scanfolderName;

            if(!platformConfiguration->ConvertToRelativePath(m_absolutePath.c_str(), databaseSourceName, scanfolderName))
            {
                AZ_Error("SourceIdentifier", false, "Failed to convert path %s to a relative path", m_absolutePath.c_str());
            }

            m_relativePath = databaseSourceName.toUtf8().constData();
            m_scanfolderPath = scanfolderName.toUtf8().constData();
        }

        AZ::IO::PathView AbsolutePath() const
        {
            return m_absolutePath;
        }

        AZ::IO::PathView RelativePath() const
        {
            return m_relativePath;
        }

        AZ::IO::PathView ScanfolderPath() const
        {
            return m_scanfolderPath;
        }

    private:
        AZ::IO::Path m_scanfolderPath;
        AZ::IO::Path m_relativePath;
        AZ::IO::Path m_absolutePath;
    };

    class UuidManager :
        public AZ::Interface<IUuidRequests>::Registrar
    {
    public:
        AZ_RTTI(UuidManager, "{49FA0129-7272-4256-A5C6-D789C156E6BA}", IUuidRequests);

        static constexpr const char* UuidKey = "UUID";

        explicit UuidManager(const PlatformConfiguration* platformConfiguration)
            : m_platformConfiguration(platformConfiguration)
        {
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            CachedUuid::Reflect(context);
        }

        AZ::Uuid GetUuidAbsPath(AZ::IO::PathView absolutePath) override
        {
            // TODO: Switch over to m_uuid
            return GetOrCreateUuidEntry(SourceIdentifier(absolutePath, m_platformConfiguration)).m_legacyUuid;
        }

        AZ::Uuid GetUuidRelPathAndScanfolder(AZ::IO::PathView scanfolderPath, AZ::IO::PathView relativePath) override
        {
            // TODO: Switch over to m_uuid
            return GetOrCreateUuidEntry(SourceIdentifier(scanfolderPath, relativePath, m_platformConfiguration)).m_legacyUuid;
        }

        AZ::Uuid GetLegacyUuidAbsPath(AZ::IO::PathView absolutePath) override
        {
            return GetOrCreateUuidEntry(SourceIdentifier(absolutePath, m_platformConfiguration)).m_legacyUuid;
        }

        AZ::Uuid GetLegacyUuidRelPathAndScanfolder(AZ::IO::PathView scanfolderPath, AZ::IO::PathView relativePath) override
        {
            return GetOrCreateUuidEntry(SourceIdentifier(scanfolderPath, relativePath, m_platformConfiguration)).m_legacyUuid;
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

        AZ::IO::Path GetAbsolutePath(AZ::IO::PathView scanfolderPath, AZ::IO::PathView relativePath)
        {
            return AZ::IO::Path(scanfolderPath) / relativePath;
        }

        AZStd::string GetCanonicalPathKey(AZ::IO::PathView file)
        {
            // TODO: Verify if this needs to be lowercased or not

            return file.LexicallyNormal().FixedMaxPathStringAsPosix().c_str();
        }

        CachedUuid GetOrCreateUuidEntry(const SourceIdentifier& sourceIdentifier)
        {
            if(sourceIdentifier.AbsolutePath().empty())
            {
                AZ_Error("UuidManager", false, "Invalid source path");
                return {};
            }

            AZStd::scoped_lock scopeLock(m_uuidMutex);

            auto key = GetCanonicalPathKey(sourceIdentifier.AbsolutePath());
            auto itr = m_uuids.find(key);

            if (itr != m_uuids.end())
            {
                return itr->second;
            }

            auto value = GetMetadataManager()->Get(sourceIdentifier.AbsolutePath(), UuidKey);

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
            newUuid.m_legacyUuid = CreateLegacyUuid(sourceIdentifier.RelativePath());

            GetMetadataManager()->Set(sourceIdentifier.AbsolutePath(), UuidKey, AZStd::any(newUuid));

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

        AZ::Uuid CreateLegacyUuid(AZ::IO::PathView sourceName, bool caseInsensitive = true)
        {
            AZStd::string lowerVersion = sourceName.FixedMaxPathStringAsPosix().c_str();

            if (caseInsensitive)
            {
                AZStd::to_lower(lowerVersion.begin(), lowerVersion.end());
            }

            AzFramework::StringFunc::Replace(lowerVersion, '\\', '/');
            return AZ::Uuid::CreateName(lowerVersion.c_str());
        }

        AZStd::recursive_mutex m_uuidMutex;
        AZStd::unordered_map<AZStd::string, CachedUuid> m_uuids;

        const PlatformConfiguration* m_platformConfiguration{};
        AzToolsFramework::IMetadataRequests* m_metadataManager{};
    };
    #pragma optimize("", on)
}
