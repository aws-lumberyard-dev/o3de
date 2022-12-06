--------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

function GetMaterialPropertyDependencies()
    return {"isTransparent", "castShadows"}
end

function Process(context)
    
    isTransparent = context:GetMaterialTypePropertyValue_bool("isTransparent")
    castShadows = context:GetMaterialTypePropertyValue_bool("castShadows")
    
    local depthPass = context:GetShaderByTag("depth")
    local shadowPass = context:GetShaderByTag("shadow")
    local forwardPass = context:GetShaderByTag("forward")
    local transparentPass = context:GetShaderByTag("transparent")
    
    depthPass:SetEnabled(not isTransparent)
    shadowPass:SetEnabled(not isTransparent and castShadows)
    forwardPass:SetEnabled(not isTransparent)
    transparentPass:SetEnabled(isTransparent)

end
