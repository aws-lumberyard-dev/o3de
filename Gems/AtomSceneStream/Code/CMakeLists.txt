
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/AtomSceneStream/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/AtomSceneStream/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the AtomSceneStream.Static target
# Note: We include the common files and the platform specific files which are set in atomscenestream_common_files.cmake
# and in ${pal_dir}/atomscenestream_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME AtomSceneStream.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atomscenestream_files.cmake
        ${pal_dir}/atomscenestream_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_RHI.Public
            Gem::Atom_RPI.Public
)

# Here add AtomSceneStream target, it depends on the AtomSceneStream.Static
ly_add_target(
    NAME AtomSceneStream ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atomscenestream_shared_files.cmake
        ${pal_dir}/atomscenestream_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AtomSceneStream.Static
)

# By default, we will specify that the above target AtomSceneStream would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME AtomSceneStream.Clients NAMESPACE Gem TARGETS Gem::AtomSceneStream)
ly_create_alias(NAME AtomSceneStream.Servers NAMESPACE Gem TARGETS Gem::AtomSceneStream)

# If we are on a host platform, we want to add the host tools targets like the AtomSceneStream.Editor target which
# will also depend on AtomSceneStream.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AtomSceneStream.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            atomscenestream_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::AtomSceneStream.Static
    )

    ly_add_target(
        NAME AtomSceneStream.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            atomscenestream_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::AtomSceneStream.Editor.Static
    )

    # By default, we will specify that the above target AtomSceneStream would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME AtomSceneStream.Tools    NAMESPACE Gem TARGETS Gem::AtomSceneStream.Editor)
    ly_create_alias(NAME AtomSceneStream.Builders NAMESPACE Gem TARGETS Gem::AtomSceneStream.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for AtomSceneStream.Static
    if(PAL_TRAIT_ATOMSCENESTREAM_TEST_SUPPORTED)
        # We support AtomSceneStream.Tests on this platform, add AtomSceneStream.Tests target which depends on AtomSceneStream.Static
        ly_add_target(
            NAME AtomSceneStream.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                atomscenestream_files.cmake
                atomscenestream_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::AtomSceneStream.Static
        )

        # Add AtomSceneStream.Tests to googletest
        ly_add_googletest(
            NAME Gem::AtomSceneStream.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_ATOMSCENESTREAM_EDITOR_TEST_SUPPORTED)
            # We support AtomSceneStream.Editor.Tests on this platform, add AtomSceneStream.Editor.Tests target which depends on AtomSceneStream.Editor
            ly_add_target(
                NAME AtomSceneStream.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    atomscenestream_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::AtomSceneStream.Editor
            )

            # Add AtomSceneStream.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::AtomSceneStream.Editor.Tests
            )
        endif()
    endif()
endif()
