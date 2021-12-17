#include <Source/AutoGen/AutoComponentTypes.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "LBCSystemComponent.h"

namespace LBC
{
    class LBCModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LBCModule, "{ad6bacbf-f1d1-486d-9d1c-95ba6330d70f}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LBCModule, AZ::SystemAllocator, 0);

        LBCModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LBCSystemComponent::CreateDescriptor(),
            });

            CreateComponentDescriptors(m_descriptors); //< Add this line to register your projects multiplayer components
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LBCSystemComponent>(),
            };
        }
    };
}// namespace LBC

AZ_DECLARE_MODULE_CLASS(Gem_LBC, LBC::LBCModule)
