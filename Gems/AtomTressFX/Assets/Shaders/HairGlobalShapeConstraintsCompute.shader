{ 
//    "Source" : "HairGlobalShapeConstraints.azsl",
    "Source" : "HairSimulationCompute.azsl",	

    "CompilerHints" : { 
        "DxcDisableOptimizations" : false,
        "DxcGenerateDebugInfo" : true
    },

    "ProgramSettings":
    {
	  "EntryPoints":
      [
        {
          "name": "IntegrationAndGlobalShapeConstraints",
          "type": "Compute"
        }
      ]
    }   
}
