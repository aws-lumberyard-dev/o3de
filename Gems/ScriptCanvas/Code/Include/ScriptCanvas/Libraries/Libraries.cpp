/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Libraries.h"

#include <Libraries/Comparison/Comparison.h>
#include <Libraries/Core/CoreNodes.h>
#include <Libraries/Deprecated/DeprecatedNodeLibrary.h>
#include <Libraries/Logic/Logic.h>
#include <Libraries/Spawning/Spawning.h>
#include <Libraries/UnitTesting/UnitTestingLibrary.h>
#include <ScriptCanvas/Libraries/Entity/EntityFunctions.h>
#include <ScriptCanvas/Libraries/Math/AABB.h>
#include <ScriptCanvas/Libraries/Math/CRC.h>
#include <ScriptCanvas/Libraries/Math/Color.h>
#include <ScriptCanvas/Libraries/Math/MathFunctions.h>
#include <ScriptCanvas/Libraries/Math/Matrix3x3.h>
#include <ScriptCanvas/Libraries/Math/Matrix4x4.h>
#include <ScriptCanvas/Libraries/Math/OBB.h>
#include <ScriptCanvas/Libraries/Math/Plane.h>
#include <ScriptCanvas/Libraries/Math/Quaternion.h>
#include <ScriptCanvas/Libraries/Math/Transform.h>
#include <ScriptCanvas/Libraries/Math/Vector2.h>
#include <ScriptCanvas/Libraries/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Vector4.h>
#include <ScriptCanvas/Libraries/String/StringFunctions.h>

namespace ScriptCanvas
{
    static AZ::EnvironmentVariable<NodeRegistry> g_nodeRegistry;

    void InitNodeRegistry()
    {
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::EntityFunctions::EntityFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::AABBFunctions::AABBFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::ColorFunctions::ColorFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::CRCFunctions::CRCFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::MathFunctions::MathFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::MathRandoms::MathRandoms);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::Matrix3x3Functions::Matrix3x3Functions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::Matrix4x4Functions::Matrix4x4Functions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::OBBFunctions::OBBFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::PlaneFunctions::PlaneFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::QuaternionFunctions::QuaternionFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::TransformFunctions::TransformFunctions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::Vector2Functions::Vector2Functions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::Vector3Functions::Vector3Functions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::Vector4Functions::Vector4Functions);
        REGISTER_SCRIPTCANVAS_FUNCTION(ScriptCanvas::StringFunctions::StringFunctions);

        g_nodeRegistry = AZ::Environment::CreateVariable<NodeRegistry>(s_nodeRegistryName);
        using namespace Library;
        Core::InitNodeRegistry(*g_nodeRegistry);
        Math::InitNodeRegistry(*g_nodeRegistry);
        Logic::InitNodeRegistry(*g_nodeRegistry);
        Comparison::InitNodeRegistry(*g_nodeRegistry);
        Time::InitNodeRegistry(*g_nodeRegistry);
        Spawning::InitNodeRegistry(*g_nodeRegistry);
        String::InitNodeRegistry(*g_nodeRegistry);
        Operators::InitNodeRegistry(*g_nodeRegistry);

#ifndef _RELEASE
        Library::UnitTesting::InitNodeRegistry(*g_nodeRegistry);
#endif
    }

    void ResetNodeRegistry()
    {
        g_nodeRegistry.Reset();
    }

    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry()
    {
        return AZ::Environment::FindVariable<NodeRegistry>(s_nodeRegistryName);
    }

    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        using namespace Library;

        Core::Reflect(reflectContext);
        Math::Reflect(reflectContext);
        Logic::Reflect(reflectContext);
        Comparison::Reflect(reflectContext);
        Time::Reflect(reflectContext);
        Spawning::Reflect(reflectContext);
        String::Reflect(reflectContext);
        Operators::Reflect(reflectContext);

        DeprecatedNodeLibrary::Reflect(reflectContext);

#ifndef _RELEASE
        Library::UnitTesting::Reflect(reflectContext);
#endif
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        using namespace Library;

        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(Core::GetComponentDescriptors());
        
        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(Math::GetComponentDescriptors());
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        
        componentDescriptors = Logic::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Comparison::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Time::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Spawning::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = String::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Operators::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = DeprecatedNodeLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

#ifndef _RELEASE
        componentDescriptors = Library::UnitTesting::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
#endif


        return libraryDescriptors;
    }

}
