--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--

-- Usage:
--
--     local Logger = require('Scripts.TestHarness.Logger').Create("IDENTIFIER")
--     Logger(1, "foo", 2.0, Vector3(1,2,3))
--
local Logger =
{
    Create = function(id)
        return function(...)
            local s = "[" .. tostring(id) .. "] "
            for _, x in ipairs(arg) do
                s = s .. tostring(x) .. " "
            end
            Debug.Log(s)
        end
    end,
}

return Logger