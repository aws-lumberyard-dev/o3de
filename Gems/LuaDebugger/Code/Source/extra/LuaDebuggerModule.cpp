/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <VsCodeLuaDebuggerSystemComponent.h>

namespace VsCodeLuaDebugger
{
    class VsCodeLuaDebuggerModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VsCodeLuaDebuggerModule, "{9EAB39C1-20BC-42EE-9A0F-C0FA015C3721}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VsCodeLuaDebuggerModule, AZ::SystemAllocator, 0);

        VsCodeLuaDebuggerModule()
            : AZ::Module()
        {
#ifdef AZ_DEBUG_BUILD
            m_descriptors.push_back(VsCodeLuaDebuggerSystemComponent::CreateDescriptor());
#endif
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
#ifdef AZ_DEBUG_BUILD
                azrtti_typeid<VsCodeLuaDebuggerSystemComponent>(),
#endif
            };
        }
    };
}

AZ_DECLARE_MODULE_CLASS(VsCodeLuaDebugger, VsCodeLuaDebugger::VsCodeLuaDebuggerModule)
