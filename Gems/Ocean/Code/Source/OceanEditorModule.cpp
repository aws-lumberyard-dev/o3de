// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <OceanModuleInterface.h>
#include <OceanEditorSystemComponent.h>

namespace Ocean
{
    class OceanEditorModule
        : public OceanModuleInterface
    {
    public:
        AZ_RTTI(OceanEditorModule, "{2e2f6a6c-edbb-4de2-8023-35b756ed77fe}", OceanModuleInterface);
        AZ_CLASS_ALLOCATOR(OceanEditorModule, AZ::SystemAllocator, 0);

        OceanEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OceanEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<OceanEditorSystemComponent>(),
            };
        }
    };
}// namespace Ocean

AZ_DECLARE_MODULE_CLASS(Gem_Ocean, Ocean::OceanEditorModule)
