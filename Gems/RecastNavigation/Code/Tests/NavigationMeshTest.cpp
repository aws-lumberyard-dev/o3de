/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MockInterfaces.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzTest/AzTest.h>
#include <Components/DetourNavigationComponent.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Components/RecastNavigationTiledSurveyorComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace RecastNavigationTests
{
    using namespace AZ;
    using namespace AZStd;
    using namespace RecastNavigation;
    using namespace testing;

    class NavigationTest
        : public ::UnitTest::AllocatorsFixture
    {
    public:
        unique_ptr<SerializeContext> m_sc;
        unique_ptr<ComponentDescriptor> m_nd1;
        unique_ptr<ComponentDescriptor> m_nd2;
        unique_ptr<ComponentDescriptor> m_nd3;
        unique_ptr<ComponentDescriptor> m_nd4;
        unique_ptr<ComponentDescriptor> m_nd5;
        unique_ptr<TimeSystem> m_timeSystem;
        unique_ptr<MockSceneInterface> m_mockSceneInterface;
        unique_ptr<AzPhysics::SceneQueryHit> m_hit;
        unique_ptr<MockPhysicsShape> m_mockPhysicsShape;
        unique_ptr<MockSimulatedBody> m_mockSimulatedBody;

        void SetUp() override
        {
            ::UnitTest::AllocatorsFixture::SetUp();

            // register components involved in testing
            m_sc = AZStd::make_unique<SerializeContext>();
            RegisterComponent<RecastNavigationMeshComponent>(m_nd1);
            RegisterComponent<RecastNavigationTiledSurveyorComponent>(m_nd2);
            RegisterComponent<MockShapeComponent>(m_nd3);
            RegisterComponent<EventSchedulerSystemComponent>(m_nd4);
            RegisterComponent<DetourNavigationComponent>(m_nd5);

            m_timeSystem = AZStd::make_unique<StubTimeSystem>();
            m_mockSceneInterface = AZStd::make_unique< NiceMock<MockSceneInterface>>();
            m_hit = AZStd::make_unique<AzPhysics::SceneQueryHit>();
            m_mockPhysicsShape = AZStd::make_unique<NiceMock<MockPhysicsShape>>();
            m_mockSimulatedBody = AZStd::make_unique<NiceMock<MockSimulatedBody>>();
        }

        void TearDown() override
        {
            m_mockSimulatedBody = {};
            m_mockPhysicsShape = {};
            m_hit = {};
            m_mockSceneInterface = {};
            m_timeSystem = {};
            m_nd1 = {};
            m_nd2 = {};
            m_nd3 = {};
            m_nd4 = {};
            m_nd5 = {};
            m_sc = {};

            ::UnitTest::AllocatorsFixture::TearDown();
        }

        // helper method
        void PopulateEntity(Entity& e)
        {
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<EventSchedulerSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();
            e.CreateComponent<RecastNavigationTiledSurveyorComponent>();
            e.CreateComponent<RecastNavigationMeshComponent>(RecastNavigationMeshConfig{}, true);

            ON_CALL(*m_mockShapeComponent, GetEncompassingAabb()).WillByDefault([]() {
                return AZ::Aabb::CreateCenterHalfExtents(
                    AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne() * 10);
                });
        }

        void SetupNavigationMesh()
        {
            m_hit->m_resultFlags = AzPhysics::SceneQuery::EntityId;
            m_hit->m_entityId = AZ::EntityId{ 1 };
            m_hit->m_shape = m_mockPhysicsShape.get();

            ON_CALL(*m_mockSceneInterface, QueryScene(_, _)).WillByDefault(Invoke([this]
            (AzPhysics::SceneHandle, const AzPhysics::SceneQueryRequest* request)
                {
                    const AzPhysics::OverlapRequest* overlapRequest = static_cast<const AzPhysics::OverlapRequest*>(request);
                    overlapRequest->m_unboundedOverlapHitCallback({ *m_hit });
                    return AzPhysics::SceneQueryHits();
                }));


            ON_CALL(*m_mockSceneInterface, GetSimulatedBodyFromHandle(_, _)).WillByDefault(Invoke([this]
            (AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle)
                {
                    return m_mockSimulatedBody.get();
                }));
            ON_CALL(*m_mockSimulatedBody, GetOrientation()).WillByDefault(Return(AZ::Quaternion::CreateIdentity()));
            ON_CALL(*m_mockSimulatedBody, GetPosition()).WillByDefault(Return(AZ::Vector3::CreateZero()));
        }

        void ActivateEntity(Entity& e)
        {
            // Bring the entity online
            e.Init();
            e.Activate();
        }

        template <typename T>
        void RegisterComponent(unique_ptr<ComponentDescriptor>& descriptor)
        {
            descriptor.reset(T::CreateDescriptor());
            descriptor->Reflect(m_sc.get());
        }

        MockShapeComponent* m_mockShapeComponent = nullptr;

        void AddTestGeometry(AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, bool indexed = true)
        {
            constexpr float size = 2.5f;
            const vector<AZ::Vector3> boxVertices = {
                AZ::Vector3(-size, -size, -size),
                AZ::Vector3(size, -size, -size) ,
                AZ::Vector3(size, size, -size)  ,
                AZ::Vector3(-size, size, -size) ,
                AZ::Vector3(-size, -size, size) ,
                AZ::Vector3(size, -size, size)  ,
                AZ::Vector3(size, size, size)   ,
                AZ::Vector3(-size, size, size)
            };
            vertices.clear();
            vertices.insert(vertices.begin(), boxVertices.begin(), boxVertices.end());

            indices.clear();
            if (indexed)
            {
                const vector<AZ::u32> boxIndices = {
                    /*0*/	2,                    /*1*/	    1,                    /*2*/	    0,
                    /*3*/	0,                    /*4*/	    3,                    /*5*/	    2,
                    /*6*/	3,                    /*7*/	    0,                    /*8*/	    7,
                    /*9*/	0,                    /*10*/	4,                    /*11*/	7,
                    /*12*/	0,                    /*13*/	1,                    /*14*/	5,
                    /*15*/	0,                    /*16*/	5,                    /*17*/	4,
                    /*18*/	1,                    /*19*/	2,                    /*20*/	5,
                    /*21*/	6,                    /*22*/	5,                    /*23*/	2,
                    /*24*/	7,                    /*25*/	2,                    /*26*/	3,
                    /*27*/	7,                    /*28*/	6,                    /*29*/	2,
                    /*30*/	7,                    /*31*/	4,                    /*32*/	5,
                    /*33*/	7,                    /*34*/	5,                    /*35*/	6,
                };
                indices.insert(indices.begin(), boxIndices.begin(), boxIndices.end());

                indices.push_back(2);
                indices.push_back(1);
                indices.push_back(0);
            }
        }
    };

    TEST_F(NavigationTest, BlockingTest)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);
    }

    TEST_F(NavigationTest, BlockingTestNonIndexedGeometry)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, false);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        TickBus::Broadcast(&TickBus::Events::OnTick, 0.1f, ScriptTimePoint{});
    }

    TEST_F(NavigationTest, TickingDebugDraw)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        class MockDebug : public AzFramework::DebugDisplayRequestBus::Handler
        {
        public:
            MockDebug()
            {
                AzFramework::DebugDisplayRequestBus::Handler::BusConnect(AzFramework::g_defaultSceneEntityDebugDisplayId);
            }

            ~MockDebug() override
            {
                AzFramework::DebugDisplayRequestBus::Handler::BusDisconnect();
            }
        };

        MockDebug debug;
        TickBus::Broadcast(&TickBus::Events::OnTick, 0.1f, ScriptTimePoint{});
    }

    TEST_F(NavigationTest, Async)
    {
        Entity e;
        PopulateEntity(e);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);

        struct Wait : public RecastNavigationMeshNotificationBus::Handler
        {
            Wait()
            {
                RecastNavigationMeshNotificationBus::Handler::BusConnect(AZ::EntityId{ 1 });
            }

            void OnNavigationMeshUpdated(AZ::EntityId) override
            {
                m_calls++;
            }

            int m_calls = 0;
        };

        const Wait wait;
        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshAsync);

        while (wait.m_calls == 0)
        {
        }
    }

    TEST_F(NavigationTest, FindPathTestDetaultDetourSettings)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>();
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        vector<Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2.f, 2, 0));

        EXPECT_GT(waypoints.size(), 0);
    }

    TEST_F(NavigationTest, FindPathTest)
    {
        Entity e;
        PopulateEntity(e);
        e.CreateComponent<DetourNavigationComponent>(e.GetId(), 3.f);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        RecastNavigationMeshRequestBus::Event(e.GetId(), &RecastNavigationMeshRequests::UpdateNavigationMeshBlockUntilCompleted);

        vector<Vector3> waypoints;
        DetourNavigationRequestBus::EventResult(waypoints, AZ::EntityId(1), &DetourNavigationRequests::FindPathBetweenPositions,
            AZ::Vector3(0.f, 0, 0), AZ::Vector3(2.f, 2, 0));

        EXPECT_GT(waypoints.size(), 0);
    }
}
