

#include <LuaDebuggerModuleInterface.h>
#include <LuaDebuggerSystemComponent.h>

namespace LuaDebugger
{
    class LuaDebuggerModule
        : public LuaDebuggerModuleInterface
    {
    public:
        AZ_RTTI(LuaDebuggerModule, "{42fb134a-bb48-4bc8-ae30-07968fb9e52a}", LuaDebuggerModuleInterface);
        AZ_CLASS_ALLOCATOR(LuaDebuggerModule, AZ::SystemAllocator, 0);
    };
}// namespace LuaDebugger

AZ_DECLARE_MODULE_CLASS(Gem_LuaDebugger, LuaDebugger::LuaDebuggerModule)
