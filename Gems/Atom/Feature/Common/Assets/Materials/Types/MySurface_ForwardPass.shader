{
    "Source" : "./MySurface_ForwardPass.azsl",

    "DepthStencilState" :
    {
        "Depth" :
        {
            "Enable" : true,
            "CompareFunc" : "GreaterEqual"
        },
        "Stencil" :
        {
            "Enable" : true,
            "ReadMask" : "0x00",
            "WriteMask" : "0xFF",
            "FrontFace" :
            {
                "Func" : "Always",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            },
            "BackFace" :
            {
                "Func" : "Always",
                "DepthFailOp" : "Keep",
                "FailOp" : "Keep",
                "PassOp" : "Replace"
            }
        }
    },

    "ProgramSettings":
    {
      "EntryPoints":
      [
        {
          "name": "MySurface_ForwardPassVS",
          "type": "Vertex"
        },
        {
          "name": "MySurface_ForwardPassPS",
          "type": "Fragment"
        }
      ]
    },

    "DrawList" : "forward"
}

