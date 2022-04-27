/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Entity/EntityFunctions.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class ScriptCanvasUnitTestEntityFunctions
        : public ScriptCanvasUnitTestFixture
        , public AZ::TransformBus::Handler
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_dummyLocalTransform = AZ::Transform::CreateIdentity();
            m_dummyWordTransform = AZ::Transform::CreateIdentity();
            m_dummyEntity.Init();
            m_dummyEntity.Activate();

            AZ::TransformBus::Handler::BusConnect(m_dummyId);
            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            m_dummyEntity.Deactivate();
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
            AZ::TransformBus::Handler::BusDisconnect(m_dummyId);

            ScriptCanvasUnitTestFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationBus
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return &m_dummyEntity; }
        AZ::SerializeContext* GetSerializeContext() override { return nullptr; }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformBus
        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override {}
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override {}
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override {}
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override {}
        const AZ::Transform& GetLocalTM() override { return m_dummyLocalTransform; }
        bool IsStaticTransform() override { return false; }
        const AZ::Transform& GetWorldTM() override { return m_dummyWordTransform; }
        void SetWorldTM(const AZ::Transform& tm) override { m_dummyWordTransform = tm; }
        //////////////////////////////////////////////////////////////////////////

        AZ::EntityId m_dummyId{123};
        AZ::Transform m_dummyLocalTransform;
        AZ::Transform m_dummyWordTransform;
        AZ::Entity m_dummyEntity;
    };

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityRight_Call_GetExpectedResult)
    {
        float dummyScale = 123.f;
        auto actualResult = EntityFunctions::GetEntityRight(m_dummyId, dummyScale);
        EXPECT_EQ(actualResult, AZ::Vector3(dummyScale, 0, 0));
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityForward_Call_GetExpectedResult)
    {
        float dummyScale = 123.f;
        auto actualResult = EntityFunctions::GetEntityForward(m_dummyId, dummyScale);
        EXPECT_EQ(actualResult, AZ::Vector3(0, dummyScale, 0));
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityUp_Call_GetExpectedResult)
    {
        float dummyScale = 123.f;
        auto actualResult = EntityFunctions::GetEntityUp(m_dummyId, dummyScale);
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, dummyScale));
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, Rotate_Call_GetExpectedResult)
    {
        auto dummyRotation = AZ::Vector3(180, 0, 0);
        EntityFunctions::Rotate(m_dummyId, AZ::Vector3(180, 0, 0));
        EXPECT_EQ(m_dummyWordTransform.GetRotation(), AZ::ConvertEulerDegreesToQuaternion(dummyRotation));
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, IsActive_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::IsActive(m_dummyId);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, IsValid_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::IsValid(m_dummyId);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, ToString_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::ToString(m_dummyId);
        EXPECT_EQ(actualResult, m_dummyId.ToString());
    }
}
