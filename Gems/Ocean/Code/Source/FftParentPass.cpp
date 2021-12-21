/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FftParentPass.h>

#include <Ocean/OceanBus.h>

#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace Ocean
{
    namespace
    {
        constexpr uint32_t LocalWorkGroupsY = 8;
    }

    AZ::RPI::Ptr<FftParentPass> FftParentPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<FftParentPass> pass = aznew FftParentPass(descriptor);
        return AZStd::move(pass);
    }

    FftParentPass::FftParentPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ParentPass(descriptor)
    {
        OceanRequestBus::BroadcastResult(m_gaussianNoiseImage, &OceanRequests::GetGaussianNoiseImage);

        const FftPassData* fftPassData = AZ::RPI::PassUtils::GetPassData<FftPassData>(descriptor);
        if (fftPassData)
        {
            m_size = fftPassData->m_size;
        }
        uint32_t logSize = aznumeric_cast<uint32_t>(log2f(aznumeric_cast<float>(m_size)));

        // Create the image for the precomputed fft data.

        AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create2D(
            AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite,
            logSize, m_size,
            AZ::RHI::Format::R32G32B32A32_FLOAT
        );
        
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        
        // The ImageViewDescriptor must be specified to make sure the frame graph compiler doesn't treat this as a transient image.
        AZ::RHI::ImageViewDescriptor viewDesc = AZ::RHI::ImageViewDescriptor::Create(imageDesc.m_format, 0, 0);
        viewDesc.m_aspectFlags = AZ::RHI::ImageAspectFlags::Color;

        // The full path name is needed for the attachment image so it's not deduplicated from images in different pipelines.
        AZStd::string imageName = AZ::RPI::ConcatPassString(GetPathName(), AZ::Name("FftPrecomputeBuffer"));
        m_fftPrecomputeImage = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(imageName), nullptr, &viewDesc);

        // Create the compute pass for evaluating the precompute data

        const char* shaderFilePath = "Shaders/FftPrecomputeCS.azshader";
        AZ::Data::AssetId shaderAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            shaderAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
            shaderFilePath, azrtti_typeid<AZ::RPI::ShaderAsset>(), false);
        if (!shaderAssetId.IsValid())
        {
            AZ_Error("FftParentPass", false, "Unable to obtain asset id for %s.", shaderFilePath);
        }

        auto computePassData = AZStd::make_shared<AZ::RPI::ComputePassData>();
        computePassData->m_shaderReference.m_assetId = shaderAssetId;
        computePassData->m_shaderReference.m_filePath = shaderFilePath;
        computePassData->m_totalNumberOfThreadsX = logSize;
        computePassData->m_totalNumberOfThreadsY = m_size / 2 / LocalWorkGroupsY;
        computePassData->m_totalNumberOfThreadsZ = 1;

        AZ::RPI::PassDescriptor passDescriptor;
        passDescriptor.m_passName = "FftPrecomputePass";
        passDescriptor.m_passData = computePassData;

        m_fftPrecomputePass = AZ::RPI::ComputePass::Create(passDescriptor);
        AddChild(m_fftPrecomputePass);
    }

    void FftParentPass::FrameBeginInternal(FramePrepareParams params [[maybe_unused]])
    {
        // Must match the struct in FftComputePass.azsl
        struct FftConstants
        {
            uint32_t m_size;
        } fftConstants;

        fftConstants.m_size = m_size;

        if (m_fftPrecomputePass)
        {
            auto precomputeSrg = m_fftPrecomputePass->GetShaderResourceGroup();
            precomputeSrg->SetConstant(m_constantsIndex, fftConstants);
            precomputeSrg->SetImage(m_precomputeImageIndex, m_fftPrecomputeImage);
        }
    }

    void FftParentPass::FrameEndInternal()
    {
        if (m_fftPrecomputePass)
        {
            RemoveChild(m_fftPrecomputePass);
            m_fftPrecomputePass = {};
        }
    }

}
