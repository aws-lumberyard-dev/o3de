// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <OceanSystemComponent.h>

namespace Ocean
{
    class OceanModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(OceanModuleInterface, "{41eaa5f7-a817-4889-8d80-d0b8f71675bf}", AZ::Module);
        AZ_CLASS_ALLOCATOR(OceanModuleInterface, AZ::SystemAllocator, 0);

        OceanModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OceanSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<OceanSystemComponent>(),
            };
        }
    };
}// namespace Ocean
