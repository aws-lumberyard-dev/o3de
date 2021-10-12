/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceGradientListComponent.h>
#include <Terrain/MockTerrainLayerSpawner.h>
#include <GradientSignal/Ebuses/MockGradientRequestBus.h>

using ::testing::NiceMock;
using ::testing::Return;

namespace UnitTest
{
    class TerrainSurfaceGradientListTest : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;

        const AZStd::string surfaceTag1 = "testtag1";
        const AZStd::string surfaceTag2 = "testtag2";

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            auto entity = AZStd::make_unique<AZ::Entity>();
            entity->Init();
            return entity;
        }

        UnitTest::MockTerrainLayerSpawnerComponent* AddRequiredComponentsToEntity(AZ::Entity* entity)
        {
            auto layerSpawnerComponent = entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
            m_app.RegisterComponentDescriptor(layerSpawnerComponent->CreateDescriptor());

            return layerSpawnerComponent;
        }

    };

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientMissingRequirementsActivateFails)
    {
        auto entity = CreateEntity();

        auto terrainSurfaceGradientListComponent = entity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>();
        m_app.RegisterComponentDescriptor(terrainSurfaceGradientListComponent->CreateDescriptor());

        const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());

        entity.reset();
    }

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientReturnsSurfaceWeightsInOrder)
    {
        auto entity = CreateEntity();

        AddRequiredComponentsToEntity(entity.get());

        // When there is more that one surface/weight defined and added to the component, they should all
        // be returned in descending weight order.

        auto gradientEntity1 = CreateEntity();
        auto gradientEntity2 = CreateEntity();

        Terrain::TerrainSurfaceGradientListConfig config;

        Terrain::TerrainSurfaceGradientMapping mapping1;
        mapping1.m_gradientEntityId = gradientEntity1->GetId();
        mapping1.m_surfaceTag = SurfaceData::SurfaceTag(surfaceTag1);
        config.m_gradientSurfaceMappings.emplace_back(mapping1);

        Terrain::TerrainSurfaceGradientMapping mapping2;
        mapping2.m_gradientEntityId = gradientEntity2->GetId();
        mapping2.m_surfaceTag = SurfaceData::SurfaceTag(surfaceTag2);
        config.m_gradientSurfaceMappings.emplace_back(mapping2);

        auto terrainSurfaceGradientListComponent = entity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>(config);
        m_app.RegisterComponentDescriptor(terrainSurfaceGradientListComponent->CreateDescriptor());

        entity->Activate();
        gradientEntity1->Activate();
        gradientEntity2->Activate();

        const float gradient1Value = 0.3f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests1(gradientEntity1->GetId());
        ON_CALL(mockGradientRequests1, GetValue).WillByDefault(Return(gradient1Value));

        const float gradient2Value = 1.0f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests2(gradientEntity2->GetId());
        ON_CALL(mockGradientRequests2, GetValue).WillByDefault(Return(gradient2Value));

        AzFramework::SurfaceData::OrderedSurfaceTagWeightSet weightSet;
        Terrain::TerrainAreaSurfaceRequestBus::Event(
            entity->GetId(), &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeights, AZ::Vector3::CreateZero(), weightSet);

        AZ::Crc32 expectedCrcList[] = { AZ::Crc32(surfaceTag2), AZ::Crc32(surfaceTag1) };
        const float expectedWeightList[] = { gradient2Value, gradient1Value };

        int index = 0;
        for (const auto& surfaceWeight : weightSet)
        {
            EXPECT_EQ(surfaceWeight.m_surfaceType, expectedCrcList[index]);
            EXPECT_NEAR(surfaceWeight.m_weight, expectedWeightList[index], 0.01f);
            index++;
        }

        gradientEntity2.reset();
        gradientEntity1.reset();
        entity.reset();
    }
} // namespace UnitTest


