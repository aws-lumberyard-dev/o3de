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
#include <AzCore/Console/Console.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <Components/DetourNavigationComponent.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <Components/RecastNavigationTiledSurveyorComponent.h>
#include <Components/Editor/EditorDetourNavigationComponent.h>
#include <Components/Editor/EditorRecastNavigationMeshComponent.h>
#include <Components/Editor/EditorRecastNavigationTiledSurveyorComponent.h>

namespace RecastNavigation
{
    using namespace AZ;
    using namespace AZStd;
    using namespace RecastNavigationTests;
    using namespace testing;

    class EditorNavigationTest
        : public ::UnitTest::AllocatorsFixture
    {
    public:
        unique_ptr<SerializeContext> m_sc;
        unique_ptr<BehaviorContext> m_bc;
        unique_ptr<ComponentDescriptor> m_nd1;
        unique_ptr<ComponentDescriptor> m_nd2;
        unique_ptr<ComponentDescriptor> m_nd3;
        unique_ptr<ComponentDescriptor> m_nd4;
        unique_ptr<ComponentDescriptor> m_nd5;
        unique_ptr<MockTimeSystem> m_timeSystem;
        unique_ptr<MockSceneInterface> m_mockSceneInterface;
        unique_ptr<AzPhysics::SceneQueryHit> m_hit;
        unique_ptr<MockPhysicsShape> m_mockPhysicsShape;
        unique_ptr<MockSimulatedBody> m_mockSimulatedBody;
        unique_ptr<AZ::Console> m_console;

        void SetUp() override
        {
            ::UnitTest::AllocatorsFixture::SetUp();

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            // register components involved in testing
            m_sc = AZStd::make_unique<SerializeContext>();
            m_sc->CreateEditContext();
            m_bc = AZStd::make_unique<BehaviorContext>();
            RegisterComponent<EditorRecastNavigationMeshComponent>(m_nd1);
            RegisterComponent<EditorRecastNavigationTiledSurveyorComponent>(m_nd2);
            RegisterComponent<MockShapeComponent>(m_nd3);
            RegisterComponent<EventSchedulerSystemComponent>(m_nd4);
            RegisterComponent<EditorDetourNavigationComponent>(m_nd5);

            m_timeSystem = AZStd::make_unique<NiceMock<MockTimeSystem>>();
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
            m_bc = {};

            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console = {};
            ::UnitTest::AllocatorsFixture::TearDown();
        }

        template <typename T>
        void RegisterComponent(unique_ptr<ComponentDescriptor>& descriptor)
        {
            descriptor.reset(T::CreateDescriptor());
            descriptor->Reflect(m_sc.get());
            descriptor->Reflect(m_bc.get());
        }

        // helper method
        void PopulateEntity(Entity& e)
        {
            e.SetId(AZ::EntityId{ 1 });
            e.CreateComponent<EventSchedulerSystemComponent>();
            m_mockShapeComponent = e.CreateComponent<MockShapeComponent>();
            e.CreateComponent<EditorRecastNavigationTiledSurveyorComponent>();
            //e.CreateComponent<EditorRecastNavigationMeshComponent>();
            e.CreateComponent<EditorDetourNavigationComponent>();
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

        EditorRecastNavigationMeshComponent* CreateEditorRecastNavigationMeshComponent(bool debugDraw, bool autoUpdate)
        {
            const char* outJson =
                "{"
                "    \"name\": \"ObjectStream\","
                "    \"version\": 3,"
                "    \"Objects\": ["
                "        {"
                "            \"typeName\": \"EditorRecastNavigationMeshComponent\","
                "            \"typeId\": \"{22D516D4-C98D-4783-85A4-1ABE23CAB4D4}\","
                "            \"version\": 1,"
                "            \"Objects\": ["
                "                {"
                "                    \"field\": \"Debug Draw\","
                "                    \"typeName\": \"bool\","
                "                    \"typeId\": \"{A0CA880C-AFE4-43CB-926C-59AC48496112}\","
                "                    \"value\": \"%s\""
                "                },"
                "                {"
                "                    \"field\": \"AutoUpdate in Editor\","
                "                    \"typeName\": \"bool\","
                "                    \"typeId\": \"{A0CA880C-AFE4-43CB-926C-59AC48496112}\","
                "                    \"value\": \"%s\""
                "                }"
                "            ]"
                "        }"
                "    ]"
                "}";

            AZStd::string templateData = AZStd::string::format(outJson, (debugDraw ? "true" : "false"), (autoUpdate ? "true" : "false"));

            AZStd::vector<AZ::u8> buffer(templateData.begin(), templateData.end());

            EditorRecastNavigationMeshComponent* outComponent = nullptr;
            const ObjectStream::ClassReadyCB readyCallback([&outComponent](void* classPtr, const Uuid& classId, const SerializeContext* context)
                {
                    outComponent = context->Cast<EditorRecastNavigationMeshComponent*>(classPtr, classId);
                });

            IO::ByteContainerStream stream(&buffer);
            if (ObjectStream::LoadBlocking(&stream, *m_sc, readyCallback) && outComponent)
            {
                return outComponent;
            }

            return nullptr;
        }

        void SetEditorMeshConfig(EditorRecastNavigationMeshComponent* component, bool debugDraw, bool autoUpdate)
        {
            component->m_enableDebugDraw = debugDraw;
            component->m_enableAutoUpdateInEditor = autoUpdate;

            component->OnAutoUpdateChanged();
            component->OnDebugDrawChanged();
        }
    };

    TEST_F(EditorNavigationTest, InEditor)
    {
        Entity e;
        PopulateEntity(e);

        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);

        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        editorMeshComponent->OnUpdateEvent();
        
        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_calls, 1);
    }

    TEST_F(EditorNavigationTest, InEditorDebugDrawTick)
    {
        Entity e;
        PopulateEntity(e);

        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);

        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        editorMeshComponent->OnUpdateEvent();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_calls, 1);

        TickBus::Broadcast(&TickBus::Events::OnTick, 0.1f, ScriptTimePoint{});
    }

    TEST_F(EditorNavigationTest, InEditorDebugDrawTickStopDebugDraw)
    {
        Entity e;
        PopulateEntity(e);

        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);

        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));

        const Wait wait(AZ::EntityId(1));
        editorMeshComponent->OnUpdateEvent();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_calls, 1);

        TickBus::Broadcast(&TickBus::Events::OnTick, 0.1f, ScriptTimePoint{});

        SetEditorMeshConfig(editorMeshComponent, false, false);
    }

    TEST_F(EditorNavigationTest, InEditorSecondRun)
    {
        Entity e;
        PopulateEntity(e);

        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);

        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();

        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));
        
        ON_CALL(*m_timeSystem, GetElapsedTimeMs()).WillByDefault(Return(AZ::TimeMs{2000}));

        {
            const Wait wait(AZ::EntityId(1));
            TickBus::Broadcast(&TickBus::Events::OnTick, 1.1f, ScriptTimePoint{});
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_calls, 1);
        }
        {
            const Wait wait(AZ::EntityId(1));
            TickBus::Broadcast(&TickBus::Events::OnTick, 1.1f, ScriptTimePoint{});
            wait.BlockUntilCalled();
            EXPECT_EQ(wait.m_calls, 1);
        }
    }

    TEST_F(EditorNavigationTest, InEditorEmptyWorld)
    {
        Entity e;
        PopulateEntity(e);

        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);

        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();

        const Wait wait(AZ::EntityId(1));
        editorMeshComponent->OnUpdateEvent();

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_calls, 1);
    }

    TEST_F(EditorNavigationTest, DeactivateRightAfterUpdateEvent)
    {
        Entity e;
        PopulateEntity(e);
        EditorRecastNavigationMeshComponent* editorMeshComponent = CreateEditorRecastNavigationMeshComponent(true, true);
        e.AddComponent(editorMeshComponent);
        ActivateEntity(e);
        SetupNavigationMesh();
        ON_CALL(*m_mockPhysicsShape.get(), GetGeometry(_, _, _)).WillByDefault(Invoke([this]
        (AZStd::vector<AZ::Vector3>& vertices, AZStd::vector<AZ::u32>& indices, AZ::Aabb*)
            {
                AddTestGeometry(vertices, indices, true);
            }));
        const Wait wait(AZ::EntityId(1));
        editorMeshComponent->OnUpdateEvent();

        e.Deactivate(); // the expectation is that that update is running on a thread as we are deactivate here

        wait.BlockUntilCalled();
        EXPECT_EQ(wait.m_calls, 1);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorRecastNavigationTiledSurveyorComponent)
    {
        Entity inEntity;
        EditorRecastNavigationTiledSurveyorComponent* inComponent = inEntity.CreateComponent<EditorRecastNavigationTiledSurveyorComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<RecastNavigationTiledSurveyorComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorRecastNavigationMeshComponent)
    {
        Entity inEntity;
        EditorRecastNavigationMeshComponent* inComponent = inEntity.CreateComponent<EditorRecastNavigationMeshComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<RecastNavigationMeshComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, BuildGameEntityFromEditorDetourNavigationComponent)
    {
        Entity inEntity;
        EditorDetourNavigationComponent* inComponent = inEntity.CreateComponent<EditorDetourNavigationComponent>();

        Entity outEntity;
        inComponent->BuildGameEntity(&outEntity);

        EXPECT_NE(outEntity.FindComponent<DetourNavigationComponent>(), nullptr);
    }

    TEST_F(EditorNavigationTest, EditorRecastNavigationTiledSurveyorComponentTestCollectGeometryNonAsyncIsDisabled)
    {
        EditorRecastNavigationTiledSurveyorComponent component;
        const AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles = component.CollectGeometry(1, 1);

        EXPECT_EQ(tiles.size(), 0);
    }
}
