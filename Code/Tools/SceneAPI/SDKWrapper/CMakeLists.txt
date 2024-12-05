#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

# Hack in assimp
set(ASSIMP_BUILD_USD_IMPORTER ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

add_subdirectory("D:/assimp/" assimp)

#target_include_directories(Test_Assimp PRIVATE "D:/assimp/include/")

#target_link_libraries(Test_Assimp PUBLIC assimp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /wd4777 /wd5032")

set_target_properties(assimp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

set(sdkwrapper_dir ${CMAKE_CURRENT_LIST_DIR}/../SDKWrapper)

ly_add_target(
    NAME SDKWrapper STATIC
    NAMESPACE AZ
    FILES_CMAKE
        sdkwrapper_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ../..
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzToolsFramework
        PUBLIC
            assimp
)
