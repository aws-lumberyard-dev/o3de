
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/LuaDebugger/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/LuaDebugger/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_name}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Add the LuaDebugger.Static target
# Note: We include the common files and the platform specific files which are set in luadebugger_common_files.cmake
# and in ${pal_dir}/luadebugger_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME LuaDebugger.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        luadebugger_files.cmake
        ${pal_dir}/luadebugger_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
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

# Here add LuaDebugger target, it depends on the LuaDebugger.Static
ly_add_target(
    NAME LuaDebugger ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        luadebugger_shared_files.cmake
        ${pal_dir}/luadebugger_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::LuaDebugger.Static
)

# By default, we will specify that the above target LuaDebugger would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME LuaDebugger.Clients NAMESPACE Gem TARGETS Gem::LuaDebugger)
ly_create_alias(NAME LuaDebugger.Servers NAMESPACE Gem TARGETS Gem::LuaDebugger)

# If we are on a host platform, we want to add the host tools targets like the LuaDebugger.Editor target which
# will also depend on LuaDebugger.Static
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME LuaDebugger.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            luadebugger_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::LuaDebugger.Static
    )

    ly_add_target(
        NAME LuaDebugger.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            luadebugger_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::LuaDebugger.Editor.Static
    )

    # By default, we will specify that the above target LuaDebugger would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME LuaDebugger.Tools    NAMESPACE Gem TARGETS Gem::LuaDebugger.Editor)
    ly_create_alias(NAME LuaDebugger.Builders NAMESPACE Gem TARGETS Gem::LuaDebugger.Editor)


endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for LuaDebugger.Static
    if(PAL_TRAIT_LUADEBUGGER_TEST_SUPPORTED)
        # We support LuaDebugger.Tests on this platform, add LuaDebugger.Tests target which depends on LuaDebugger.Static
        ly_add_target(
            NAME LuaDebugger.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                luadebugger_files.cmake
                luadebugger_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::LuaDebugger.Static
        )

        # Add LuaDebugger.Tests to googletest
        ly_add_googletest(
            NAME Gem::LuaDebugger.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_LUADEBUGGER_EDITOR_TEST_SUPPORTED)
            # We support LuaDebugger.Editor.Tests on this platform, add LuaDebugger.Editor.Tests target which depends on LuaDebugger.Editor
            ly_add_target(
                NAME LuaDebugger.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    luadebugger_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::LuaDebugger.Editor
            )

            # Add LuaDebugger.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::LuaDebugger.Editor.Tests
            )
        endif()
    endif()
endif()
