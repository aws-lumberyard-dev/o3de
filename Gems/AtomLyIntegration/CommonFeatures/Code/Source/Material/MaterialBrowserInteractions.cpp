/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/wildcard.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <Material/MaterialBrowserInteractions.h>

namespace AZ
{
    namespace Render
    {
        MaterialBrowserInteractions::MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        }

        MaterialBrowserInteractions::~MaterialBrowserInteractions()
        {
            AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        }

        void MaterialBrowserInteractions::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
        {
            if (HandlesSource(fullSourceFileName))
            {
                openers.push_back({ "Material_Editor", "Open in Material Editor...", QIcon(),
                    [&](const char* fullSourceFileNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                            fullSourceFileNameInCallback);
                    } });
            }
        }

        void MaterialBrowserInteractions::AddSourceFileCreators([[maybe_unused]] const char* fullSourceFolderName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            creators.push_back({ "Material_Creator", "Material", QIcon(),
                [&](const char* fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                        AZStd::string::format("--create-path=%s",  fullSourceFolderNameInCallback).c_str());
                } });
        }

        bool MaterialBrowserInteractions::HandlesSource(AZStd::string_view fileName) const
        {
            return AZStd::wildcard_match("*.material", fileName.data());
        }
    } // namespace Render
} // namespace AZ
