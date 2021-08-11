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

#include <Code/src/TressFX/TressFXCommon.h>

#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairRenderObject.h>
#include <Passes/HairSkinningComputePass.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {

            HairDispatchItem::~HairDispatchItem()
            {
            }

            // Reference in the code above that tackles handling of the different dispatches possible
            // This one is targeting the per vertex dispatches fro now.
            void HairDispatchItem::InitSkinningDispatch(
                RPI::Shader* shader,
                RPI::ShaderResourceGroup* hairGenerationSrg,
                RPI::ShaderResourceGroup* hairSimSrg,
                uint32_t elementsAmount )
            {
                m_shader = shader;
                RHI::DispatchDirect dispatchArgs(
                    elementsAmount, 1, 1,
                    TRESSFX_SIM_THREAD_GROUP_SIZE, 1, 1
                );
                m_dispatchItem.m_arguments = dispatchArgs;
                RHI::PipelineStateDescriptorForDispatch pipelineDesc;
                m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId).ConfigurePipelineState(pipelineDesc);
                m_dispatchItem.m_pipelineState = m_shader->AcquirePipelineState(pipelineDesc);
                m_dispatchItem.m_shaderResourceGroupCount = 2;      // the per pass will be added by each pass.
                m_dispatchItem.m_shaderResourceGroups = {
                    hairGenerationSrg->GetRHIShaderResourceGroup(), // Static generation data
                    hairSimSrg->GetRHIShaderResourceGroup()         // Dynamic data changed between passes
                };
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
