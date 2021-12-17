#include <Source/AutoGen/AutoComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/RTTI/BehaviorContext.h>

#include "LBCSystemComponent.h"

namespace LBC
{
    void LBCSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LBCSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LBCSystemComponent>("LBC", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LBCRequestBus>("LBCRequestBus", "")
                ->Attribute(AZ::Script::Attributes::Category, "LBC")
                ->Event("SetRedBallEntityId", &LBCRequestBus::Events::SetRedBallEntityId)
                ->Event("GetRedBallEntityId", &LBCRequestBus::Events::GetRedBallEntityId)
                ->Event("SetBlueBallEntityId", &LBCRequestBus::Events::SetBlueBallEntityId)
                ->Event("GetBlueBallEntityId", &LBCRequestBus::Events::GetBlueBallEntityId);
        }
    }

    void LBCSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LBCService"));
    }

    void LBCSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LBCService"));
    }

    void LBCSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void LBCSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void LBCSystemComponent::SetRedBallEntityId(AZ::EntityId entityId)
    {
        m_redBallEntityId = entityId;
    }

    AZ::EntityId LBCSystemComponent::GetRedBallEntityId() const
    {
        return m_redBallEntityId;
    }

    void LBCSystemComponent::SetBlueBallEntityId(AZ::EntityId entityId)
    {
        m_blueBallEntityId = entityId;
    }

    AZ::EntityId LBCSystemComponent::GetBlueBallEntityId() const
    {
        return m_blueBallEntityId;
    }
    
    LBCSystemComponent::LBCSystemComponent()
    {
        if (LBCInterface::Get() == nullptr)
        {
            LBCInterface::Register(this);
        }

        LBCRequestBus::Handler::BusConnect();
    }

    LBCSystemComponent::~LBCSystemComponent()
    {
        LBCRequestBus::Handler::BusDisconnect();

        if (LBCInterface::Get() == this)
        {
            LBCInterface::Unregister(this);
        }
    }

    void LBCSystemComponent::Init()
    {
    }

    void LBCSystemComponent::Activate()
    {
        RegisterMultiplayerComponents(); //< Register our gems multiplayer components to assign NetComponentIds
    }

    void LBCSystemComponent::Deactivate()
    {
    }
}
