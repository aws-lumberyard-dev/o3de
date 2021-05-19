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

#include <AWSGameLiftServerMocks.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class AWSGameLiftServerSystemComponentTest
        : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            ScopedAllocatorSetupFixture::SetUp();

            m_componentDescriptor.reset(AWSGameLift::AWSGameLiftServerSystemComponent::CreateDescriptor());
            m_componentDescriptor->Reflect(m_serializeContext.get());

            m_entity = aznew AZ::Entity();

            m_AWSGameLiftServerSystemsComponent = aznew NiceMock<AWSGameLiftServerSystemComponentMock>();
            m_entity->AddComponent(m_AWSGameLiftServerSystemsComponent);

            // Set up the file IO and alias
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            m_localFileIO->SetAlias("@log@", AZ_TRAIT_TEST_ROOT_FOLDER);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            m_entity->RemoveComponent(m_AWSGameLiftServerSystemsComponent);
            delete m_AWSGameLiftServerSystemsComponent;
            delete m_entity;

            m_serializeContext.reset();
            m_componentDescriptor.reset();

            ScopedAllocatorSetupFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;

        AZ::Entity* m_entity;      
        NiceMock<AWSGameLiftServerSystemComponentMock>* m_AWSGameLiftServerSystemsComponent;

        AZ::IO::FileIOBase* m_priorFileIO;
        AZ::IO::FileIOBase* m_localFileIO;
    };

    TEST_F(AWSGameLiftServerSystemComponentTest, ActivateDeactivateComponent_ExecuteInOrder_Success)
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


} // namespace UnitTest
