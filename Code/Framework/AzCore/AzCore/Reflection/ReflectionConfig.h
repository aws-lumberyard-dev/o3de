/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

// The following code is part of a prototype for a new reflection system partly driven by code gen. Development was put on hold to refocus
// on the first public release of O3DE and to allow for more time to collect feedback from various interested parties. To allow
// interested parties to take a look at the code for reference, the prototype implementation was left available.
#define AZ_REFLECTION_PROTOTYPE_ENABLED 0

// Example of a xml description of a reflectable class
// <?xml version="1.0" encoding="utf-8"?>
// 
// <Reflection namespace="LmbrCentral">
//     <Include path="AzToolsFramework/ToolsComponents/EditorComponentBase.h" in="header" />
//     <Include path="Source/Editor/EditorCommentComponent.reflection.h" in="body" />
// 
//     <Struct name="EditorCommentComponentData" flags="ReflectFunction" extends="AzToolsFramework::Components::EditorComponentBase">
//         <Attribute group="Class" name="Name" value="Editor Comment Data"/>
//         <Attribute group="Class" name="Description" value="The data for the editor comment."/>
//         <Variable name="Comment" type="String">
//             <Attribute group="Variable" name="Description" value="The comment text to store."/>
//             <Attribute group="PropertyGrid" name="Editor" value="MultiLineEdit"/>
//             <Attribute group="PropertyGrid" name="HideName">
//                 <Value>true</Value>
//             </Attribute>
//             <Attribute group="PropertyGrid" name="PlaceholderText" value="Add comment text here"/>
//         </Variable>
//     </Struct>
// </Reflection>

// Example for including the code gen into a project:
// In CMakeLists.txt:
//      BUILD_DEPENDENCIES
//          PUBLIC
//              AZ::AzCore
//      AUTOGEN_RULES
//          *.reflection.xml,Reflection.h.jinja,$path/$fileprefix.reflection.h
//          *.reflection.xml,Reflection.cpp.jinja,$path/$fileprefix.reflection.cpp
//
// In the projects .cmake file:
//      ${LY_ROOT_FOLDER}/Code/Framework/AzCore/AzCore/Reflection/AutoGen/Reflection.h.jinja
//      ${LY_ROOT_FOLDER}/Code/Framework/AzCore/AzCore/Reflection/AutoGen/Reflection.cpp.jinja
// and a xml file with the above description and with a filename ending with ".reflection.xml".
