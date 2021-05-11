/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AWSCoreBus.h>
#include <AWSGameLiftClientSystemComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace AWSGameLiftClientUnitTest
{
    class AWSGameLiftClientSystemComponentMock
        : public AWSGameLift::AWSGameLiftClientSystemComponent
    {
    public:
        void InitMock()
        {
            AWSGameLift::AWSGameLiftClientSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSGameLift::AWSGameLiftClientSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSGameLift::AWSGameLiftClientSystemComponent::Deactivate();
        }

        AWSGameLiftClientSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSGameLiftClientSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSGameLiftClientSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSGameLiftClientSystemComponentMock::DeactivateMock));
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());  
    };

    class AWSCoreSystemComponentMock
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(AWSCoreSystemComponentMock, "{72e81433-313f-41a1-bc29-96cc0fc29658}");
 
        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<AWSCoreSystemComponentMock, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AWSCoreSystemComponentMock>("AWSCoreMock", "Adds core support for working with AWS")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AWSCoreService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            AZ_UNUSED(incompatible);
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void ActivateMock()
        {
            AWSCore::AWSCoreNotificationsBus::Broadcast(&AWSCore::AWSCoreNotifications::OnSDKInitialized);
        }

        AWSCoreSystemComponentMock()
        {
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSCoreSystemComponentMock::ActivateMock));
        }

        ~AWSCoreSystemComponentMock() = default;

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
    };
}

class AWSGameLiftClientSystemComponentTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
protected:
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_awsCoreComponentDescriptor;

    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;

    void SetUp() override
    {
        m_componentDescriptor.reset(AWSGameLift::AWSGameLiftClientSystemComponent::CreateDescriptor());
        m_awsCoreComponentDescriptor.reset(AWSGameLiftClientUnitTest::AWSCoreSystemComponentMock::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_awsCoreComponentDescriptor->Reflect(m_serializeContext.get());

        m_entity = aznew AZ::Entity();

        m_AWSGameLiftClientSystemsComponent = aznew testing::NiceMock<AWSGameLiftClientUnitTest::AWSGameLiftClientSystemComponentMock>();
        m_awsCoreSystemsComponent = aznew testing::NiceMock<AWSGameLiftClientUnitTest::AWSCoreSystemComponentMock>();
        m_entity->AddComponent(m_awsCoreSystemsComponent);
        m_entity->AddComponent(m_AWSGameLiftClientSystemsComponent);
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_AWSGameLiftClientSystemsComponent);
        m_entity->RemoveComponent(m_awsCoreSystemsComponent);
        delete m_awsCoreSystemsComponent;
        delete m_AWSGameLiftClientSystemsComponent;
        delete m_entity;

        m_componentDescriptor.reset();
        m_awsCoreComponentDescriptor.reset();
    }

public:
    testing::NiceMock<AWSGameLiftClientUnitTest::AWSGameLiftClientSystemComponentMock> *m_AWSGameLiftClientSystemsComponent;
    testing::NiceMock<AWSGameLiftClientUnitTest::AWSCoreSystemComponentMock> *m_awsCoreSystemsComponent;
    AZ::Entity* m_entity = nullptr;
};



TEST_F(AWSGameLiftClientSystemComponentTest, ActivateDeactivate_Success)
{
    testing::Sequence s1, s2;

    EXPECT_CALL(*m_awsCoreSystemsComponent, Init()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_AWSGameLiftClientSystemsComponent, Init()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_awsCoreSystemsComponent, Activate()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_AWSGameLiftClientSystemsComponent, Activate()).Times(1).InSequence(s1);

    EXPECT_CALL(*m_AWSGameLiftClientSystemsComponent, Deactivate()).Times(1).InSequence(s2);
    EXPECT_CALL(*m_awsCoreSystemsComponent, Deactivate()).Times(1).InSequence(s2);

    // activate component
    m_entity->Init();
    m_entity->Activate();

    // deactivate component
    m_entity->Deactivate();
}
