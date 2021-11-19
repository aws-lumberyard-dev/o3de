import json
import os
import random

materialConfig_level1_materialCount = 100

materialConfig_level2_materialCountPerBranch = 10
materialConfig_level2_branchCount = 3

materialConfig_level3_materialCountPerBranch = 2
materialConfig_level3_branchCount = 5

########################################################################################

class MaterialInhertianceLevel:

    def __init__(self, createMaterialCallbackFunction, childCoundPerBranch, branchCount = None):
        self.branchCount = branchCount
        self.childCoundPerBranch = childCoundPerBranch
        self.createMaterialCallbackFunction = createMaterialCallbackFunction
        self.nextMaterialInhertianceLevel = None


    def setNextLevel(self, nextMaterialInhertianceLevel):
        self.nextMaterialInhertianceLevel = nextMaterialInhertianceLevel
        

    def generateMaterialFiles(self, materialsFolder):
        generatedMaterialFilenames = self._generateMaterialFilesInFolder(materialsFolder, self.childCoundPerBranch, 'Materials/Types/StandardPBR.materialtype', '')
        
        if self.nextMaterialInhertianceLevel:
            self.nextMaterialInhertianceLevel._generateChildMaterialFiles(generatedMaterialFilenames)


    def _generateChildMaterialFiles(self, upperLevelMaterials):

        for i in range(0, self.branchCount):
            # When only a few of the upperLevelMaterials are to be parents, we evenly space the selection of those materials to get more variation in their inherited properties
            maxBranchIndex = self.branchCount - 1
            parentMaterialIndex = int((len(upperLevelMaterials)-1) * i / maxBranchIndex)

            parentMaterialFilePath = upperLevelMaterials[parentMaterialIndex]
            parentMaterialFilename = os.path.split(parentMaterialFilePath)[1]

            # The children will go in a folder that has the same name as the parent material
            childMaterialsFolder = os.path.splitext(parentMaterialFilePath)[0] 

            # We use '/' instead of os.path.join because that's commonly used in o3de json files, even on Windows
            parentMaterialReferece = '../' + parentMaterialFilename

            childMaterialFilenames = self._generateMaterialFilesInFolder(childMaterialsFolder, self.childCoundPerBranch, 'Materials/Types/StandardPBR.materialtype', parentMaterialReferece)
            
            # Recurse to next level of the material inhertiance hierarchy
            if self.nextMaterialInhertianceLevel:
                self.nextMaterialInhertianceLevel._generateChildMaterialFiles(childMaterialFilenames)

        
    def _generateMaterialFilesInFolder(self, folder, howMany, materialType, parentMaterial):

        if(not os.path.exists(folder)):
            os.mkdir(folder)

        materialFilenameList = []

        for i in range(0, howMany):
            # The third parameter is a generic factor argument that ranges from 0.0-1.0 as more materials are generated in a single folder; 
            # each callback can use this factor as it sees fit, or not at all.
            factor = i / max(howMany-1, 1)

            material = self.createMaterialCallbackFunction(parentMaterial, materialType, factor)
            jsonText = json.dumps(material, indent='    ')

            filePath = '{}/m{:0>4}.material'.format(folder, i)
            materialFilenameList.append(filePath)

            with open(filePath, 'w') as f:
                f.write(jsonText)

        return materialFilenameList
        

        
########################################################################################
# Create Material Functions...

# These are callback functions for making one material. The third parameter is a generic factor argument that ranges from 0.0-1.0 as 
# more materials are generated in a single folder; each callback can use this factor as it sees fit, or not at all.

def createRoughnessMaterial(parentMaterial, materialType, roughness):
    material = {} 
    material['materialType'] = materialType
    material['parentMaterial'] = parentMaterial
    material['properties'] = { 'roughness': { 'factor': roughness } }
    return material

def createRandomColorMaterial(parentMaterial, materialType, unused):
    material = {} 
    material['materialType'] = materialType
    material['parentMaterial'] = parentMaterial
    material['properties'] = { 'baseColor': { 'color': [ random.random(), random.random(), random.random() ] } }
    return material
    
########################################################################################

def askYesNo(question):
    while True:
        print(question + ' (y/n)')
        response = input().lower()
        if(response == 'y'): return True
        if(response == 'n'): return False

def main():

    # We force the data to be exactly the same every time the script is run to easily reproduce the same data set on multiple computers
    random.seed(0)

    rootFolder = 'generated'
    if(os.path.exists(rootFolder)):
        if(not askYesNo('This will replace all previously generated data. Continue?'.format(rootFolder))):
            return
    else:
        os.mkdir(rootFolder)
            
    materialsLevel1 = MaterialInhertianceLevel(createRoughnessMaterial, materialConfig_level1_materialCount)
    materialsLevel2 = MaterialInhertianceLevel(createRandomColorMaterial, materialConfig_level2_materialCountPerBranch, materialConfig_level2_branchCount)
    materialsLevel3 = MaterialInhertianceLevel(createRoughnessMaterial, materialConfig_level3_materialCountPerBranch, materialConfig_level3_branchCount)

    materialsLevel1.setNextLevel(materialsLevel2)
    materialsLevel2.setNextLevel(materialsLevel3)

    materialsLevel1.generateMaterialFiles(os.path.join(rootFolder, 'materials'))

main()