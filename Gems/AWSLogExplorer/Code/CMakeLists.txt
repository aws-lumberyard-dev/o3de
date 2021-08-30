#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

ly_add_target(
    NAME AWSLogExplorer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awslogexplorer_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Here add AWSLogExplorer target, it depends on the AWSLogExplorer.Static
ly_add_target(
    NAME AWSLogExplorer ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        awslogexplorer_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AWSLogExplorer.Static
)

ly_create_alias(NAME AWSLogExplorer.Clients NAMESPACE Gem TARGETS Gem::AWSLogExplorer)
ly_create_alias(NAME AWSLogExplorer.Servers NAMESPACE Gem TARGETS Gem::AWSLogExplorer)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_create_alias(NAME AWSLogExplorer.Tools    NAMESPACE Gem TARGETS Gem::AWSLogExplorer)
    ly_create_alias(NAME AWSLogExplorer.Builders NAMESPACE Gem TARGETS Gem::AWSLogExplorer)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AWSLogExplorer.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            awslogexplorer_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzCore
                AZ::AzFramework
                Gem::AWSLogExplorer.Static
    )
    # Add AWSLogExplorer.Tests to googletest
    ly_add_googletest(
        NAME Gem::AWSLogExplorer.Tests
    )
endif()
