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

#include <AWSGameLiftServerSystemComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace AWSGameLiftServerUnitTest
{
    class AWSGameLiftServerSystemComponentMock
        : public AWSGameLift::AWSGameLiftServerSystemComponent
    {
    public:
        void InitMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Deactivate();
        }

        AWSGameLiftServerSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::DeactivateMock));
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());  
    };
}

class AWSGameLiftServerSystemComponentTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
protected:
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;

    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;

    void SetUp() override
    {
        m_componentDescriptor.reset(AWSGameLift::AWSGameLiftServerSystemComponent::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());

        m_entity = aznew AZ::Entity();

        m_AWSGameLiftServerSystemsComponent = aznew testing::NiceMock<AWSGameLiftServerUnitTest::AWSGameLiftServerSystemComponentMock>();
        m_entity->AddComponent(m_AWSGameLiftServerSystemsComponent);
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_AWSGameLiftServerSystemsComponent);
        delete m_AWSGameLiftServerSystemsComponent;
        delete m_entity;

        m_componentDescriptor.reset();
    }

public:
    testing::NiceMock<AWSGameLiftServerUnitTest::AWSGameLiftServerSystemComponentMock> *m_AWSGameLiftServerSystemsComponent;
    AZ::Entity* m_entity = nullptr;
};



TEST_F(AWSGameLiftServerSystemComponentTest, ActivateDeactivate_Success)
{
    testing::Sequence s1, s2;

    EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Init()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Activate()).Times(1).InSequence(s1);

    EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Deactivate()).Times(1).InSequence(s2);

    // activate component
    m_entity->Init();
    m_entity->Activate();

    // deactivate component
    m_entity->Deactivate();
}
