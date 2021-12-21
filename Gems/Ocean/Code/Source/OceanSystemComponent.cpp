// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <OceanSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>

#include <Atom/Utils/DdsFile.h>

#include <random>

namespace Ocean
{
    void OceanSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OceanSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<OceanSystemComponent>("Ocean", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void OceanSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
        required.push_back(AZ_CRC_CE("CommonService"));
        required.push_back(AZ_CRC_CE("BootstrapSystemComponent"));
    }

    void OceanSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    OceanSystemComponent::OceanSystemComponent()
    {
        if (OceanInterface::Get() == nullptr)
        {
            OceanInterface::Register(this);
        }
    }

    OceanSystemComponent::~OceanSystemComponent()
    {
        if (OceanInterface::Get() == this)
        {
            OceanInterface::Unregister(this);
        }
    }

    void OceanSystemComponent::Init()
    {
    }

    void OceanSystemComponent::Activate()
    {
        OceanRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        auto passSystem = AZ::RPI::PassSystemInterface::Get();
        if (!passSystem)
        {
            AZ_Error("OceanSystemComponent", false, "Unable to load pass system to initialize ocean pass templates.")
            return;
        }

        // Setup handler for load pass templates mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            [&]()
            {
                const char* passTemplatesFile = "Passes/OceanPassTemplates.azasset";
                passSystem->LoadPassTemplateMappings(passTemplatesFile);
            });
        passSystem->ConnectEvent(m_loadTemplatesHandler);

    }

    void OceanSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        OceanRequestBus::Handler::BusDisconnect();
    }

    AZ::Data::Instance<AZ::RPI::StreamingImage> OceanSystemComponent::GetGaussianNoiseImage()
    {
        if (!m_gaussianNoise256Image)
        {
            m_gaussianNoise256Image = AZ::RPI::StreamingImage::FindOrCreate(CreateGaussian256ImageAsset());
        }
        return m_gaussianNoise256Image;
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> OceanSystemComponent::CreateGaussian256ImageAsset()
    {
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> streamingImageAsset;

        // Build the random noise data
        constexpr uint32_t NoiseTextureSize = 256;

        AZStd::vector<float> noiseTexture;
        noiseTexture.resize_no_construct(NoiseTextureSize * NoiseTextureSize);
        const uint32_t sizeInBytes = aznumeric_cast<uint32_t>(noiseTexture.size() * sizeof(float));

        std::default_random_engine generator;
        std::normal_distribution<float> distribution(0.0f, 1.0f);
        float total = 0.0;

        for (uint32_t i = 0; i < noiseTexture.size(); ++i)
        {
            noiseTexture.at(i) = distribution(generator);
            total += noiseTexture.at(i);
        }

        // Set up descriptor / layout
        AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D
            (AZ::RHI::ImageBindFlags::ShaderRead, NoiseTextureSize, NoiseTextureSize, AZ::RHI::Format::R32_FLOAT);
        
        const AZ::RHI::ImageSubresourceLayout imageSubresourceLayout = AZ::RHI::GetImageSubresourceLayout(imageDescriptor, AZ::RHI::ImageSubresource{});
        const size_t expectedImageDataSize = imageSubresourceLayout.m_bytesPerImage * imageDescriptor.m_size.m_depth;
        AZ_Assert(expectedImageDataSize == sizeInBytes, "Expected image size incorrect");
        
        // Construct the mip chain asset.
        AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> mipChainAsset;
        {
            AZ::RPI::ImageMipChainAssetCreator assetCreator;
            assetCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateName("GaussianNoise256x256Mip"), 0), 1, 1);
            assetCreator.BeginMip(imageSubresourceLayout);
            assetCreator.AddSubImage(noiseTexture.data(), expectedImageDataSize);
            assetCreator.EndMip();
            if (!assetCreator.End(mipChainAsset))
            {
                AZ_Error("StreamingImage", false, "Failed to initialize mip chain asset");
                return streamingImageAsset;
            }
        }

        // Construct the streaming image asset
        {
            AZ::RPI::StreamingImageAssetCreator imageAssetCreator;
            imageAssetCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateName("GaussianNoise256x256Image")));
            imageAssetCreator.SetImageDescriptor(imageDescriptor);
            imageAssetCreator.AddMipChainAsset(*mipChainAsset.Get());
            imageAssetCreator.End(streamingImageAsset);
        }
        streamingImageAsset.SetHint("GaussianNoise256x256");

        return streamingImageAsset;
    }

    void OceanSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace Ocean
