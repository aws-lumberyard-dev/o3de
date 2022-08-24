#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# The Clouds.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME Clouds.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        clouds_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The Clouds.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME Clouds.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        clouds_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
)

ly_add_target(
    NAME Clouds ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        clouds_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::Clouds.API
        PRIVATE
            Gem::Clouds.Private.Object
)

# By default, we will specify that the above target Clouds would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME Clouds.Clients NAMESPACE Gem TARGETS Gem::Clouds)
ly_create_alias(NAME Clouds.Servers NAMESPACE Gem TARGETS Gem::Clouds)

# For the Client and Server variants of Clouds Gem, an alias to the Clouds.API target will be made
ly_create_alias(NAME Clouds.Clients.API NAMESPACE Gem TARGETS Gem::Clouds.API)
ly_create_alias(NAME Clouds.Servers.API NAMESPACE Gem TARGETS Gem::Clouds.API)

# If we are on a host platform, we want to add the host tools targets like the Clouds.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The Clouds.Editor.API target can be used by other gems that want to interact with the Clouds.Editor module
    ly_add_target(
        NAME Clouds.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            clouds_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The Clouds.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME Clouds.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            clouds_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework

                Gem::Clouds.Private.Object
    )

    ly_add_target(
        NAME Clouds.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            clouds_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Clouds.Editor.API
            PRIVATE
                Gem::Clouds.Editor.Private.Object
    )

    # By default, we will specify that the above target Clouds would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME Clouds.Tools    NAMESPACE Gem TARGETS Gem::Clouds.Editor)
    ly_create_alias(NAME Clouds.Builders NAMESPACE Gem TARGETS Gem::Clouds.Editor)

    # For the Tools and Builders variants of Clouds Gem, an alias to the Clouds.Editor API target will be made
    ly_create_alias(NAME Clouds.Tools.API NAMESPACE Gem TARGETS Gem::Clouds.Editor.API)
    ly_create_alias(NAME Clouds.Builders.API NAMESPACE Gem TARGETS Gem::Clouds.Editor.API)

endif()

