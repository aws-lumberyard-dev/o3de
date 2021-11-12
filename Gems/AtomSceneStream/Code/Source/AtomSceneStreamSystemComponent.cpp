
#include <AtomSceneStreamSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <AtomSceneStreamFeatureProcessor.h>

namespace AtomSceneStream
{
    void AtomSceneStreamSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomSceneStreamSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomSceneStreamSystemComponent>("AtomSceneStream", "[AtonSceneStream handles scene stream input received via the cloud and generates 3D data that is added to the Atom render pipeline]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomSceneStreamSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomSceneStreamService"));
    }

    void AtomSceneStreamSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomSceneStreamService"));
    }

    void AtomSceneStreamSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AtomSceneStreamSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    AtomSceneStreamSystemComponent::AtomSceneStreamSystemComponent()
    {
        if (AtomSceneStreamInterface::Get() == nullptr)
        {
            AtomSceneStreamInterface::Register(this);
        }
    }

    AtomSceneStreamSystemComponent::~AtomSceneStreamSystemComponent()
    {
        if (AtomSceneStreamInterface::Get() == this)
        {
            AtomSceneStreamInterface::Unregister(this);
        }
    }

    void AtomSceneStreamSystemComponent::Init()
    {
    }

    void AtomSceneStreamSystemComponent::Activate()
    {
        // Feature processor
        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<AtomSceneStreamFeatureProcessor>();

        AtomSceneStreamRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void AtomSceneStreamSystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<AtomSceneStreamFeatureProcessor>();

        AZ::TickBus::Handler::BusDisconnect();
        AtomSceneStreamRequestBus::Handler::BusDisconnect();
    }

    void AtomSceneStreamSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace AtomSceneStream
