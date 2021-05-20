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

#include <Editor/Attribution/AWSCoreAttributionSystemComponent.h>
#include <Editor/Attribution/AWSCoreAttributionManager.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSCore
{

    AWSCoreAttributionSystemComponent::AWSCoreAttributionSystemComponent()
        : m_manager(AZStd::make_unique<AWSCoreAttributionManager>())
    {
    }

    void AWSCoreAttributionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSCoreAttributionSystemComponent, AZ::Component>()->Version(0);
            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSCoreAttributionSystemComponent>("AWSCoreAttributions", "Generates operation metrics for AWSCore gem")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void AWSCoreAttributionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSCoreAttributionService"));
    }

    void AWSCoreAttributionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSCoreAttributionService"));
    }

    void AWSCoreAttributionSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSCoreAttributionSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSCoreAttributionSystemComponent::Init()
    {
        // load config if required - ie check if attributions should be generated and pass to manager
        m_manager->Init();
    }

    void AWSCoreAttributionSystemComponent::Activate()
    {
        m_manager->MetricCheck();
    }

    void AWSCoreAttributionSystemComponent::Deactivate()
    {
    }

} // namespace AWSCore

