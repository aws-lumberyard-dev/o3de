
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <LuaDebuggerSystemComponent.h>

namespace LuaDebugger
{
    class LuaDebuggerModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(LuaDebuggerModuleInterface, "{ce1b26cd-7803-47b5-af63-a2db9a567cbd}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LuaDebuggerModuleInterface, AZ::SystemAllocator, 0);

        LuaDebuggerModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                LuaDebuggerSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LuaDebuggerSystemComponent>(),
            };
        }
    };
}// namespace LuaDebugger
