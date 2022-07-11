
#include "ONNXRuntimeSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace ONNXRuntime
{
    void ONNXRuntimeSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ONNXRuntimeSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ONNXRuntimeSystemComponent>("ONNXRuntime", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ONNXRuntimeSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ONNXRuntimeService"));
    }

    void ONNXRuntimeSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ONNXRuntimeService"));
    }

    void ONNXRuntimeSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ONNXRuntimeSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ONNXRuntimeSystemComponent::ONNXRuntimeSystemComponent()
    {
        if (ONNXRuntimeInterface::Get() == nullptr)
        {
            ONNXRuntimeInterface::Register(this);
        }
    }

    ONNXRuntimeSystemComponent::~ONNXRuntimeSystemComponent()
    {
        if (ONNXRuntimeInterface::Get() == this)
        {
            ONNXRuntimeInterface::Unregister(this);
        }
    }

    void ONNXRuntimeSystemComponent::Init()
    {
    }

    void ONNXRuntimeSystemComponent::Activate()
    {
        ONNXRuntimeRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void ONNXRuntimeSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ONNXRuntimeRequestBus::Handler::BusDisconnect();
    }

    void ONNXRuntimeSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace ONNXRuntime
