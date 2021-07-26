/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "GradientSignal_precompiled.h"
#include "EditorImageGradientComponent.h"
#include "EditorImageGradientComponentMode.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Utils/Utils.h>
#include <Atom/Utils/DdsFile.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/ImageProcessing/PixelFormats.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

namespace GradientSignal
{
    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorImageGradientComponent, BaseClassType>()
                ->Version(1)
                ->Field("ComponentMode", &EditorImageGradientComponent::m_componentModeDelegate)
                ->Field("PaintBrush", &EditorImageGradientComponent::m_paintBrush)
                ->Field("OverridePaintLayerPath", &EditorImageGradientComponent::m_path)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorImageGradientComponent>(s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorImageGradientComponent::m_paintBrush, "Paint Brush", "Paint Brush Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::InComponentMode)
                    ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorImageGradientComponent::m_path, "Override Paint Layer Path",
                        "Path to the modified paint layer.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_componentModeDelegate, "Component Mode",
                        "Image Gradient Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::InOverrideMode);
            }
        }
    }

    AZ::Crc32 EditorImageGradientComponent::InOverrideMode() const
    {
        return m_configuration.m_useOverride ? AZ::Edit::PropertyVisibility::ShowChildrenOnly : AZ::Edit::PropertyVisibility::Hide;
    }

    bool EditorImageGradientComponent::InComponentMode() const
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    void EditorImageGradientComponent::Activate()
    {
        BaseClassType::Activate();
        m_paintBrush.Activate(AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
        AzToolsFramework::PaintBrushComponentNotificationBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorImageGradientComponent, EditorImageGradientComponentMode>(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
    }

    void EditorImageGradientComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (!m_configuration.m_overrideAsset.Get())
        {
            AZ::Data::AssetId newAssetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom());
            AZ::Data::Asset<ImageAsset> imageAsset = asset;
            m_configuration.m_overrideAsset = AZ::Data::AssetManager::Instance().CreateAsset(
                newAssetId, asset.GetType(), m_configuration.m_overrideAsset.GetAutoLoadBehavior());

            m_component.CopyImageAsset(imageAsset, m_configuration.m_overrideAsset);
        }
    }

    void EditorImageGradientComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        AzToolsFramework::PaintBrushComponentNotificationBus::Handler::BusDisconnect();

        m_paintBrush.Deactivate();
        BaseClassType::Deactivate();
    }

    void EditorImageGradientComponent::OnCompositionChanged()
    {
        BaseClassType::OnCompositionChanged();

        if (!AZ::Data::AssetBus::Handler::BusIsConnectedId(m_configuration.m_imageAsset.GetId()))
        {
            m_configuration.m_useOverride = false;
            m_configuration.m_overrideAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
            m_path.clear();

            m_component.ClearOverrideConfiguration();

            AZ::Data::AssetBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_imageAsset.GetId());
        }
    }

    void EditorImageGradientComponent::SavePaintLayer()
    {
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();

        AZ::IO::FixedMaxPath relativePath = m_path;
        AZ::IO::FixedMaxPath fullPath;

        if (!relativePath.empty())
        {
            fullPath = projectPath / relativePath;
            if (!AZ::IO::FileIOBase::GetInstance()->Exists(fullPath.c_str()))
            {
                relativePath.clear();
            }
        }

        if (relativePath.empty())
        {
            auto uuidString = AZ::Uuid::CreateRandom().ToString<AZ::IO::FixedMaxPathString>() + "_gsi.dds";
            relativePath = AZ::IO::FixedMaxPath("ImageGradientOverride") / uuidString;

            auto invalidCharacters = [](char letter)
            {
                return AZStd::string_view(R"(:"'{}<>)").contains(letter);
            };
            AZStd::string relativePathStr = relativePath.String();
            AZStd::replace_if(relativePathStr.begin(), relativePathStr.end(), invalidCharacters, '_');

            relativePath = relativePathStr;
            fullPath = projectPath / relativePath;
        }

        AZ::IO::FixedMaxPath overrideFolder = fullPath.ParentPath();
        AZ::IO::SystemFile::CreateDir(overrideFolder.c_str());

        m_path = relativePath.String();
        m_configuration.m_overrideAsset.SetHint(relativePath.String());
        WriteOutputFile(fullPath.String());
        
        bool assetFound = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;

        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, relativePath.c_str(),
            sourceInfo, watchFolder);
       
        AZ::Data::AssetId newAssetId = AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, 2);
        AZ::Data::Asset<ImageAsset> asset(newAssetId, m_configuration.m_imageAsset.GetType());
        m_configuration.m_overrideAsset = asset;
        m_configuration.m_overrideAsset.QueueLoad();
    }

    void EditorImageGradientComponent::WriteOutputFile(const AZStd::string& filePath)
    {
        AZ::DdsFile::DdsFileData ddsFileData;
        const auto& image = m_configuration.m_overrideAsset.Get();

        ddsFileData.m_size.m_width = image->m_imageWidth;
        ddsFileData.m_size.m_height = image->m_imageHeight;
        ddsFileData.m_format = GetFormat(m_configuration.m_overrideAsset);
        ddsFileData.m_buffer = &image->m_imageData;

        auto outcome = AZ::DdsFile::WriteFile(filePath, ddsFileData);
        if (!outcome)
        {
            AZ_Warning("WriteDds", false, outcome.GetError().m_message.c_str());
        }
    }

    AZ::RHI::Format EditorImageGradientComponent::GetFormat(const AZ::Data::Asset<ImageAsset>& imageAsset)
    {
        switch (imageAsset.Get()->m_imageFormat)
        {
        case ImageProcessingAtom::ePixelFormat_R8:
            return AZ::RHI::Format::R8_UNORM;
        case ImageProcessingAtom::ePixelFormat_R16:
            return AZ::RHI::Format::R16_UINT;
        case ImageProcessingAtom::ePixelFormat_R32:
            return AZ::RHI::Format::R32_UINT;
        case ImageProcessingAtom::ePixelFormat_R32F:
            return AZ::RHI::Format::R32_FLOAT;
        default:
            return AZ::RHI::Format::Unknown;
        }
    }
}
