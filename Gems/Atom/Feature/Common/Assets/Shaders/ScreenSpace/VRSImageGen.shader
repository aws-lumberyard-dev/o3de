{
    "Source": "VRSImageGen.azsl", 
    "AddBuildArguments" : { "dxc" : ["-fspv-target-env=vulkan1.1"] },
    "ProgramSettings": {
        "EntryPoints": [
            {
                "name": "MainCS",
                "type": "Compute"
            }
        ]
    }
}