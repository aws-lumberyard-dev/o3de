
#include <LuaDebuggerModuleInterface.h>
#include <LuaDebuggerEditorSystemComponent.h>

namespace LuaDebugger
{
    class LuaDebuggerEditorModule
        : public LuaDebuggerModuleInterface
    {
    public:
        AZ_RTTI(LuaDebuggerEditorModule, "{42fb134a-bb48-4bc8-ae30-07968fb9e52a}", LuaDebuggerModuleInterface);
        AZ_CLASS_ALLOCATOR(LuaDebuggerEditorModule, AZ::SystemAllocator, 0);

        LuaDebuggerEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                LuaDebuggerEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<LuaDebuggerEditorSystemComponent>(),
            };
        }
    };
}// namespace LuaDebugger

AZ_DECLARE_MODULE_CLASS(Gem_LuaDebugger, LuaDebugger::LuaDebuggerEditorModule)
