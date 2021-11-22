

#include <AtomSceneStreamModuleInterface.h>
#include <AtomSceneStreamSystemComponent.h>

#pragma optimize("",off)

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamModule
            : public AtomSceneStreamModuleInterface
        {
        public:
            AZ_RTTI(AtomSceneStreamModule, "{37a6ac37-5b31-4431-b01b-41606b1386b5}", AtomSceneStreamModuleInterface);
            AZ_CLASS_ALLOCATOR(AtomSceneStreamModule, AZ::SystemAllocator, 0);

            AtomSceneStreamModule() : AtomSceneStreamModuleInterface()
            {
                m_descriptors.insert(
                    m_descriptors.end(),
                    {
                        AtomSceneStreamSystemComponent::CreateDescriptor(),
                    });
            }

        };
    } // namespace AtomSceneStream

    AZ_DECLARE_MODULE_CLASS(Gem_AtomSceneStream, AZ::AtomSceneStream::AtomSceneStreamModule)
}

#pragma optimize("",on)

