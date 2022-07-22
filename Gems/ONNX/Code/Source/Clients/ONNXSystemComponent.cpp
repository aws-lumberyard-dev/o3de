
#include "ONNXSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include "Model.h"

// This is the structure to interface with the MNIST model
// After instantiation, set the input_image_ data to be the 28x28 pixel image of the number to recognize
// Then call Run() to fill in the results_ data with the probabilities of each
// m_result holds the index with highest probability (aka the number the model thinks is in the image)
namespace ONNX
{

    void ONNXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ONNXSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ONNXSystemComponent>("ONNX", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ONNXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ONNXService"));
    }

    void ONNXSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ONNXSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }


    ONNXSystemComponent::ONNXSystemComponent()
    {
        if (ONNXInterface::Get() == nullptr)
        {
            ONNXInterface::Register(this);
        }
    }

    ONNXSystemComponent::~ONNXSystemComponent()
    {
        if (ONNXInterface::Get() == this)
        {
            ONNXInterface::Unregister(this);
        }
    }

    Ort::Env* ONNXSystemComponent::GetEnv() {
        return m_env.get();
    }

    Ort::AllocatorWithDefaultOptions* ONNXSystemComponent::GetAllocator() {
        return m_allocator.get();
    }

    void OnnxLoggingFunction(void*, OrtLoggingLevel, const char* category, const char* logid, const char* code_location, const char* message) {
        AZ_Printf("\nONNX", "%s %s %s %s", category, logid, code_location, message);
    }

    void ONNXSystemComponent::Init()
    {
        void* ptr;
        m_env = AZStd::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_VERBOSE, "test_log", OnnxLoggingFunction, ptr);
        m_allocator = AZStd::make_unique<Ort::AllocatorWithDefaultOptions>();
    }

    void ONNXSystemComponent::Activate()
    {
        ONNXRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        RunMnistSuite();
    }

    void ONNXSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ONNXRequestBus::Handler::BusDisconnect();
    }

    void ONNXSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace ONNX
