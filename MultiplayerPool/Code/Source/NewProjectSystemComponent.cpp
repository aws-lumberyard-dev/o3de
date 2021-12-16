
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "NewProjectSystemComponent.h"

namespace NewProject
{
    void NewProjectSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<NewProjectSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<NewProjectSystemComponent>("NewProject", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void NewProjectSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("NewProjectService"));
    }

    void NewProjectSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NewProjectService"));
    }

    void NewProjectSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void NewProjectSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
    
    NewProjectSystemComponent::NewProjectSystemComponent()
    {
        if (NewProjectInterface::Get() == nullptr)
        {
            NewProjectInterface::Register(this);
        }
    }

    NewProjectSystemComponent::~NewProjectSystemComponent()
    {
        if (NewProjectInterface::Get() == this)
        {
            NewProjectInterface::Unregister(this);
        }
    }

    void NewProjectSystemComponent::Init()
    {
    }

    void NewProjectSystemComponent::Activate()
    {
        NewProjectRequestBus::Handler::BusConnect();
    }

    void NewProjectSystemComponent::Deactivate()
    {
        NewProjectRequestBus::Handler::BusDisconnect();
    }
}
