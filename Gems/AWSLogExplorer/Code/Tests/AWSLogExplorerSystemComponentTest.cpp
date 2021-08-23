#include <AWSLogExplorerSystemComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace AWSLogExplorer
{
    class AWSLogExplorerSystemComponentMock : public AWSLogExplorerSystemComponent
    {

    public:
        void InitMock()
        {
            AWSLogExplorerSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSLogExplorerSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSLogExplorerSystemComponent::Deactivate();
        }

        AWSLogExplorerSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSLogExplorerSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSLogExplorerSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSLogExplorerSystemComponentMock::DeactivateMock));
        }
        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
    };

    class AWSLogExplorerSystemComponentTest : public UnitTest::ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            m_componentDescriptor.reset(AWSLogExplorer::AWSLogExplorerSystemComponent::CreateDescriptor());
            m_entity = aznew AZ::Entity();
            m_AWSLogExplorerSystemsComponent = aznew testing::NiceMock<AWSLogExplorerSystemComponentMock>();
            m_entity->AddComponent(m_AWSLogExplorerSystemsComponent);
        }
        void TearDown() override
        {
            m_entity->RemoveComponent(m_AWSLogExplorerSystemsComponent);
            delete m_AWSLogExplorerSystemsComponent;
            delete m_entity;
            m_componentDescriptor.reset();
            ScopedAllocatorSetupFixture::TearDown();
        }
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;

    public:
        testing::NiceMock<AWSLogExplorerSystemComponentMock>* m_AWSLogExplorerSystemsComponent = nullptr;
        AZ::Entity* m_entity = nullptr;
    };

    TEST_F(AWSLogExplorerSystemComponentTest, ActivateComponent_NewEntity_Success)
    {
        EXPECT_CALL(*m_AWSLogExplorerSystemsComponent, Activate()).Times(1);
        EXPECT_CALL(*m_AWSLogExplorerSystemsComponent, Init()).Times(1);
        EXPECT_CALL(*m_AWSLogExplorerSystemsComponent, Deactivate()).Times(1);
        // initialize component
        m_entity->Init();
        // activate component
        m_entity->Activate();
        // deactivate component
        m_entity->Deactivate();
    }
} // namespace AWSLogExplorer
