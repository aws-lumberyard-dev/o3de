import json
import os
import random

configMaterialCount = 3

materialConfig_level1_materialCount = 100
materialConfig_level2_materialCount = 33
materialConfig_level2_branchCount = 3

# There must be at least enough materials at each level to support the branch count of the next level
materialConfig_level1_materialCount = max(materialConfig_level1_materialCount, materialConfig_level2_branchCount)
        
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

def generateMaterialFiles(inFolder, howMany, materialGeneratorFunction, materialType, parentMaterial):

    if(not os.path.exists(inFolder)):
        os.mkdir(inFolder)

    materialFilenameList = []

    for i in range(0, howMany):
        # The third parameter is a generic factor argument that ranges from 0.0-1.0 as more materials are generated in a single folder; 
        # each callback can use this factor as it sees fit, or not at all.
        factor = i / max(howMany-1, 1)

        material = materialGeneratorFunction(parentMaterial, materialType, factor)
        jsonText = json.dumps(material, indent='    ')

        filename = 'm{:0>4}.material'.format(i)
        materialFilenameList.append(filename)

        with open(os.path.join(inFolder, filename), 'w') as f:
            f.write(jsonText)

    return materialFilenameList

def askYesNo(question):
    while True:
        print(question + ' (y/n)')
        response = input().lower()
        if(response == 'y'): return True
        if(response == 'n'): return False


def generateAllMaterialFiles(materialsFolder):

    level1GeneratedMaterialFilenames = generateMaterialFiles(materialsFolder, materialConfig_level1_materialCount, createRoughnessMaterial, 'Materials/Types/StandardPBR.materialtype', '')

    level2MaterialsPerBranch = int(materialConfig_level2_materialCount / materialConfig_level2_branchCount)
    for level2BranchIndex in range(0, materialConfig_level2_branchCount):
        # When only a few of the generated materials are to be parents, we evenly space the selection of those materials to get more variation in their inherited properties
        maxBranchIndex = materialConfig_level2_branchCount - 1
        parentMaterialIndex = int((len(level1GeneratedMaterialFilenames)-1) * level2BranchIndex / maxBranchIndex)

        parentMaterialFilename = level1GeneratedMaterialFilenames[parentMaterialIndex]

        level2MaterialsFolder = os.path.join(materialsFolder, parentMaterialFilename)
        level2MaterialsFolder = os.path.splitext(level2MaterialsFolder)[0]

        level2GeneratedMaterialFilenames = generateMaterialFiles(level2MaterialsFolder, level2MaterialsPerBranch, createRandomColorMaterial, 'Materials/Types/StandardPBR.materialtype', os.path.join('..', parentMaterialFilename))



def main():

    # We force the data to be exactly the same every time the script is run to easily reproduce the same data set on multiple computers
    random.seed(0)

    rootFolder = 'generated'
    if(os.path.exists(rootFolder)):
        if(not askYesNo('This will replace all previously generated data. Continue?'.format(rootFolder))):
            return
    else:
        os.mkdir(rootFolder)

    generateAllMaterialFiles(os.path.join(rootFolder, 'materials'));



main()