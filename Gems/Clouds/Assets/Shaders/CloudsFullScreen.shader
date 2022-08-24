{ 
    "Source" : "CloudsFullScreen.azsl",

    "DepthStencilState" : 
    {
        "Depth" : 
        { 
            "Enable" : false
        },
        "Stencil" :
        {
            "Enable" : false
        }
    },

    "BlendState" : 
    {
        "Enable" : true,

        "BlendSource" : "One",
        "BlendAlphaSource" : "One",
        "BlendDest" : "AlphaSourceInverse",
        "BlendAlphaDest" : "AlphaSourceInverse",
        "BlendAlphaOp" : "Add"

        //"BlendSource" : "One",
        //"BlendDest" : "AlphaSource",
        //"BlendOp" : "Add",
        //"BlendAlphaSource" : "Zero",
        //"BlendAlphaDest" : "Zero",
        //"BlendAlphaOp" : "Add"
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MainVS",
          "type": "Vertex"
        },
        {
          "name": "MainPS",
          "type": "Fragment"
        }
      ]
    }
}
