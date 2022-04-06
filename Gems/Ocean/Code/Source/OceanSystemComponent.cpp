
#include <OceanSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <OceanFeatureProcessor.h>
#include <Passes/OceanConjugatedSpectrumComputePass.h>
#include <Passes/OceanFftComputePass.h>
#include <Passes/OceanInitialSpectrumComputePass.h>


namespace Ocean
{
    void OceanSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        OceanFeatureProcessor::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<OceanSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<OceanSystemComponent>("Ocean", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void OceanSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("OceanService"));
    }

    void OceanSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void OceanSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    OceanSystemComponent::OceanSystemComponent()
    {
        if (OceanInterface::Get() == nullptr)
        {
            OceanInterface::Register(this);
        }
    }

    OceanSystemComponent::~OceanSystemComponent()
    {
        if (OceanInterface::Get() == this)
        {
            OceanInterface::Unregister(this);
        }
    }

    void OceanSystemComponent::Init()
    {
    }

    void OceanSystemComponent::Activate()
    {
        OceanRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<OceanFeatureProcessor>();

        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        // Setup handler for load pass templates mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
        passSystem->ConnectEvent(m_loadTemplatesHandler);

        // Register ocean system related passes
        passSystem->AddPassCreator(AZ::Name("OceanConjugatedSpectrumComputePass"), &OceanConjugatedSpectrumComputePass::Create);
        passSystem->AddPassCreator(AZ::Name("OceanInitialSpectrumComputePass"), &OceanInitialSpectrumComputePass::Create);
        passSystem->AddPassCreator(AZ::Name("OceanFftComputePass"), &OceanFftComputePass::Create);
    }

    void OceanSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        OceanRequestBus::Handler::BusDisconnect();
    }

    void OceanSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

    void OceanSystemComponent::LoadPassTemplateMappings()
    {
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");

        const char* passTemplatesFile = "Passes/OceanPassTemplates.azasset";
        passSystem->LoadPassTemplateMappings(passTemplatesFile);
    }

} // namespace Ocean
