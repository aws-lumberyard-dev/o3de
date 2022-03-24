/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Math/MathUtils.h>

namespace ScriptCanvas
{
    namespace StringNodes
    {
        static constexpr const char* k_categoryName = "String";

        AZ_INLINE Data::StringType ToLower(Data::StringType sourceString)
        {
            AZStd::to_lower(sourceString.begin(), sourceString.end());
            return sourceString;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_TRAIT(ToLower, ScriptCanvas::NoDefaultArguments, k_categoryName, "{FC5FA07E-C65D-470A-BEFA-714EF8103866}", true, "Makes all the characters in the string lower case", "Source");
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT_CONFIG(ToLower, ScriptCanvas::NoDefaultArguments, "ScriptCanvas_StringFunctions_ToLower", "Source", "String");

        AZ_INLINE Data::StringType ToUpper(Data::StringType sourceString)
        {
            AZStd::to_upper(sourceString.begin(), sourceString.end());
            return sourceString;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_TRAIT(ToUpper, ScriptCanvas::NoDefaultArguments, k_categoryName, "{323951D4-9BB1-47C9-BD2C-2DD2B750217F}", true, "Makes all the characters in the string upper case", "Source");
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT_CONFIG(ToUpper, ScriptCanvas::NoDefaultArguments, "ScriptCanvas_StringFunctions_ToUpper", "Source", "String");

        AZ_INLINE Data::StringType Substring(Data::StringType sourceString, AZ::u32 index, AZ::u32 length)
        {
            length = AZ::GetClamp<AZ::u32>(length, 0, aznumeric_cast<AZ::u32>(sourceString.size()));

            if (length == 0 || index >= sourceString.size())
            {
                return {};
            }

            return sourceString.substr(index, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE_TRAIT(Substring, ScriptCanvas::NoDefaultArguments, k_categoryName, "{031BCDFC-5DA4-4EA0-A310-1FA9165E5BE5}", true, "Returns a sub string from a given string", "Source", "From", "Length");
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT_CONFIG(Substring, ScriptCanvas::NoDefaultArguments, "ScriptCanvas_StringFunctions_Substring", "Source", "From", "Length", "String");

        using Registrar = RegistrarGeneric
            <
            ToLowerNode,
            ToUpperNode,
            SubstringNode
            >;
    }
}
