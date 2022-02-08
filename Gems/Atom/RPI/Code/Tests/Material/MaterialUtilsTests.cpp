/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/JsonTestUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialUtilsTests
        : public RPITestFixture
    {
    protected:

        void Reflect(AZ::ReflectContext* context) override
        {
            RPITestFixture::Reflect(context);
            MaterialTypeSourceData::Reflect(context);
            MaterialSourceData::Reflect(context);
        }

        void SetUp() override
        {
            EXPECT_EQ(nullptr, IO::FileIOBase::GetInstance());

            RPITestFixture::SetUp();

            auto localFileIO = AZ::IO::FileIOBase::GetInstance();
            EXPECT_NE(nullptr, localFileIO);
            const AZ::IO::FixedMaxPath exeFolder = AZ::Utils::GetExecutableDirectory();
            const AZ::IO::FixedMaxPath tempFolder = exeFolder / "temp" / "MaterialUtilsTests";
            localFileIO->SetAlias("@temp@", tempFolder.c_str());
        }

        void TearDown() override
        {
            RPITestFixture::TearDown();
        }

    };
    
    TEST_F(MaterialUtilsTests, UpgradeMaterialFile_ConvertLegacyNestingToFlatList)
    {
        const char* materialTypeJson = R"(
                {
                    "propertyLayout": {
                        "propertyGroups": [
                            {
                                "name": "general",
                                "properties": [
                                    {"name": "MyInt", "type": "Int"}
                                ]
                            }
                        ]
                    }
                }
            )";
    
        const AZStd::string originalMaterialJson = R"(
            {
                "materialType": "@temp@/test.materialtype",
                "properties": {
                    "settings": {
                        "MyInt": 5
                    }
                }
            }
        )";
        
        const AZStd::string upgradedMaterialJson = R"(
            {
                "materialType": "@temp@/test.materialtype",
                "propertyValues": {
                    "settings.MyInt": 5
                }
            }
        )";

        AZ::Utils::WriteFile(materialTypeJson, "@temp@/test.materialtype");
        AZ::Utils::WriteFile(originalMaterialJson, "@temp@/test.material");

        MaterialUtils::UpgradeMaterialFile("@temp@/test.material");
        
        ExpectSimilarJson(AZ::Utils::ReadFile("@temp@/test.material").TakeValue(), upgradedMaterialJson);
    }

    // This would require converting the loaded source data to a MaterialAsset, run the version update routines,
    // and then save again while diff-ing the material against its parent (something MaterialDocument knows how
    // to do but not MaterialUtils).
    TEST_F(MaterialUtilsTests, DISABLED_UpgradeMaterialFile_RenameProperties_WithInheritance)
    {
        const char* materialTypeJson = R"(
                {
                    "version": 10,
                    "propertyLayout": {
                        "propertyGroups": [
                            {
                                "name": "general",
                                "properties": [
                                    {"name": "MyInt1", "type": "Int"},
                                    {"name": "MyInt2", "type": "Int"}
                                ]
                            }
                        ]
                    },
                    "versionUpdates": [
                        {
                            "toVersion": 4,
                            "actions": [
                                {"op": "rename", "from": "general.YourInt1", "to": "general.MyInt1"},
                                {"op": "rename", "from": "general.YourInt2", "to": "general.MyInt2"}
                            ]
                        }
                    ]
                }
            )";
    
        const AZStd::string originalMaterialJson1 = R"(
            {
                "materialType": "@temp@/test.materialtype"
                "materialTypeVersion": 2,
                "propertyValues": {
                    "general.YourInt1": 5
                }
            }
        )";
        
        const AZStd::string originalMaterialJson2 = R"(
            {
                "materialType": "@temp@/test.materialtype"
                "materialTypeVersion": 3,
                "parentMaterial": "m1.material"
                "propertyValues": {
                    "general.YourInt2": 7
                }
            }
        )";
        
        const AZStd::string upgradedMaterialJson1 = R"(
            {
                "materialType": "@temp@/test.materialtype"
                "materialTypeVersion": 10,
                "propertyValues": {
                    "general.MyInt1": 5
                }
            }
        )";
        

        const AZStd::string upgradedMaterialJson2 = R"(
            {
                "materialType": "@temp@/test.materialtype"
                "materialTypeVersion": 10,
                "parentMaterial": "m1.material"
                "propertyValues": {
                    "general.MyInt2": 7
                }
            }
        )";
        
        AZ::Utils::WriteFile(materialTypeJson, "@temp@/test.materialtype");
        AZ::Utils::WriteFile(originalMaterialJson1, "@temp@/m1.material");
        AZ::Utils::WriteFile(originalMaterialJson2, "@temp@/m2.material");

        MaterialUtils::UpgradeMaterialFile("@temp@/m2.material");
        
        ExpectSimilarJson(AZ::Utils::ReadFile("@temp@/m1.material").TakeValue(), originalMaterialJson1);
        ExpectSimilarJson(AZ::Utils::ReadFile("@temp@/m2.material").TakeValue(), upgradedMaterialJson2);
        
        MaterialUtils::UpgradeMaterialFile("@temp@/m1.material");
        
        ExpectSimilarJson(AZ::Utils::ReadFile("@temp@/m1.material").TakeValue(), upgradedMaterialJson1);
    }

}


