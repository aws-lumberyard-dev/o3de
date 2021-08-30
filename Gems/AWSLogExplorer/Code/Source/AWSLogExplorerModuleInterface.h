/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AWSLogExplorerSystemComponent.h>

namespace AWSLogExplorer
{
    class AWSLogExplorerModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSLogExplorerModuleInterface, "{6bec5b1b-bcd5-446d-9ab0-0765025d7506}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSLogExplorerModuleInterface, AZ::SystemAllocator, 0);

        AWSLogExplorerModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AWSLogExplorerSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AWSLogExplorerSystemComponent>(),
            };
        }
    };
}// namespace AWSLogExplorer
