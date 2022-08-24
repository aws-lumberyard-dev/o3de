
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Components/CloudsSystemComponent.h>
#include <Passes/CloudsFullScreenPass.h>
#include <Rendering/CloudsFeatureProcessor.h>

namespace Clouds
{
    void CloudsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        CloudsFeatureProcessor::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudsSystemComponent>("Clouds", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void CloudsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CloudsService"));
    }

    void CloudsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CloudsService"));
    }

    void CloudsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void CloudsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    CloudsSystemComponent::CloudsSystemComponent()
    {
        if (CloudsInterface::Get() == nullptr)
        {
            CloudsInterface::Register(this);
        }
    }

    CloudsSystemComponent::~CloudsSystemComponent()
    {
        if (CloudsInterface::Get() == this)
        {
            CloudsInterface::Unregister(this);
        }
    }

    void CloudsSystemComponent::Init()
    {
    }

    void CloudsSystemComponent::Activate()
    {
        CloudsRequestBus::Handler::BusConnect();

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<CloudsFeatureProcessor>();

        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        // Setup handler for load pass templates mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
            [passSystem]()
            {
                const char* passTemplatesFile = "Passes/CloudsPassTemplates.azasset";
                passSystem->LoadPassTemplateMappings(passTemplatesFile);
            });
        passSystem->AddPassCreator(AZ::Name("CloudsFullScreenPass"), &CloudsFullScreenPass::Create);
        passSystem->ConnectEvent(m_loadTemplatesHandler);
    }

    void CloudsSystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<CloudsFeatureProcessor>();

        m_loadTemplatesHandler.Disconnect();

        CloudsRequestBus::Handler::BusDisconnect();
    }

} // namespace Clouds
