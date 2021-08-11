
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/AWSLogExplorer/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/AWSLogExplorer/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the AWSLogExplorer.Static target
# Note: We include the common files and the platform specific files which are set in awslogexplorer_common_files.cmake
# and in ${pal_dir}/awslogexplorer_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME AWSLogExplorer.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        awslogexplorer_files.cmake
        ${pal_dir}/awslogexplorer_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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
        ${pal_dir}/awslogexplorer_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AWSLogExplorer.Static
)

# By default, we will specify that the above target AWSLogExplorer would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME AWSLogExplorer.Clients NAMESPACE Gem TARGETS Gem::AWSLogExplorer)
ly_create_alias(NAME AWSLogExplorer.Servers NAMESPACE Gem TARGETS Gem::AWSLogExplorer)

# If we are on a host platform, we want to add the host tools targets like the AWSLogExplorer.Editor target which
# will also depend on AWSLogExplorer.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AWSLogExplorer.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            awslogexplorer_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::AWSLogExplorer.Static
    )

    ly_add_target(
        NAME AWSLogExplorer.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.AWSLogExplorer.Editor
        FILES_CMAKE
            awslogexplorer_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::AWSLogExplorer.Editor.Static
    )

    # By default, we will specify that the above target AWSLogExplorer would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME AWSLogExplorer.Tools    NAMESPACE Gem TARGETS Gem::AWSLogExplorer.Editor)
    ly_create_alias(NAME AWSLogExplorer.Builders NAMESPACE Gem TARGETS Gem::AWSLogExplorer.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for AWSLogExplorer.Static
    if(PAL_TRAIT_AWSLOGEXPLORER_TEST_SUPPORTED)
        # We support AWSLogExplorer.Tests on this platform, add AWSLogExplorer.Tests target which depends on AWSLogExplorer.Static
        ly_add_target(
            NAME AWSLogExplorer.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                awslogexplorer_files.cmake
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

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_AWSLOGEXPLORER_EDITOR_TEST_SUPPORTED)
            # We support AWSLogExplorer.Editor.Tests on this platform, add AWSLogExplorer.Editor.Tests target which depends on AWSLogExplorer.Editor
            ly_add_target(
                NAME AWSLogExplorer.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    awslogexplorer_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::AWSLogExplorer.Editor
            )

            # Add AWSLogExplorer.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::AWSLogExplorer.Editor.Tests
            )
        endif()
    endif()
endif()
