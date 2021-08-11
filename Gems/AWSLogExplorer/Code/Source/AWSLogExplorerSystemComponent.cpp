
#include <AWSLogExplorerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AWSLogExplorer
{
    void AWSLogExplorerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSLogExplorerSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSLogExplorerSystemComponent>("AWSLogExplorer", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSLogExplorerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSLogExplorerService"));
    }

    void AWSLogExplorerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSLogExplorerService"));
    }

    void AWSLogExplorerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AWSLogExplorerSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    AWSLogExplorerSystemComponent::AWSLogExplorerSystemComponent()
    {
        if (AWSLogExplorerInterface::Get() == nullptr)
        {
            AWSLogExplorerInterface::Register(this);
        }
    }

    AWSLogExplorerSystemComponent::~AWSLogExplorerSystemComponent()
    {
        if (AWSLogExplorerInterface::Get() == this)
        {
            AWSLogExplorerInterface::Unregister(this);
        }
    }

    void AWSLogExplorerSystemComponent::Init()
    {
    }

    void AWSLogExplorerSystemComponent::Activate()
    {
        AWSLogExplorerRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void AWSLogExplorerSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        AWSLogExplorerRequestBus::Handler::BusDisconnect();
    }

    void AWSLogExplorerSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace AWSLogExplorer
