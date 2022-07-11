
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/ONNXRuntime/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/ONNXRuntime/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# The ONNXRuntime.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ONNXRuntime.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        onnxruntime_api_files.cmake
        ${pal_dir}/onnxruntime_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The ONNXRuntime.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME ONNXRuntime.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        onnxruntime_private_files.cmake
        ${pal_dir}/onnxruntime_private_files.cmake
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
            # Add onnx library name here
)

# Here add ONNXRuntime target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ONNXRuntime ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        onnxruntime_shared_files.cmake
        ${pal_dir}/onnxruntime_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ONNXRuntime.API
        PRIVATE
            Gem::ONNXRuntime.Private.Object
)

# By default, we will specify that the above target ONNXRuntime would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ONNXRuntime.Clients NAMESPACE Gem TARGETS Gem::ONNXRuntime)
ly_create_alias(NAME ONNXRuntime.Servers NAMESPACE Gem TARGETS Gem::ONNXRuntime)

# For the Client and Server variants of ONNXRuntime Gem, an alias to the ONNXRuntime.API target will be made
ly_create_alias(NAME ONNXRuntime.Clients.API NAMESPACE Gem TARGETS Gem::ONNXRuntime.API)
ly_create_alias(NAME ONNXRuntime.Servers.API NAMESPACE Gem TARGETS Gem::ONNXRuntime.API)

# If we are on a host platform, we want to add the host tools targets like the ONNXRuntime.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ONNXRuntime.Editor.API target can be used by other gems that want to interact with the ONNXRuntime.Editor module
    ly_add_target(
        NAME ONNXRuntime.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            onnxruntime_editor_api_files.cmake
            ${pal_dir}/onnxruntime_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ONNXRuntime.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ONNXRuntime.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            onnxruntime_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                $<TARGET_OBJECTS:Gem::ONNXRuntime.Private.Object>
    )

    ly_add_target(
        NAME ONNXRuntime.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            onnxruntime_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::ONNXRuntime.Editor.API
            PRIVATE
                Gem::ONNXRuntime.Editor.Private.Object
    )

    # By default, we will specify that the above target ONNXRuntime would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ONNXRuntime.Tools    NAMESPACE Gem TARGETS Gem::ONNXRuntime.Editor)
    ly_create_alias(NAME ONNXRuntime.Builders NAMESPACE Gem TARGETS Gem::ONNXRuntime.Editor)

    # For the Tools and Builders variants of ONNXRuntime Gem, an alias to the ONNXRuntime.Editor API target will be made
    ly_create_alias(NAME ONNXRuntime.Tools.API NAMESPACE Gem TARGETS Gem::ONNXRuntime.Editor.API)
    ly_create_alias(NAME ONNXRuntime.Builders.API NAMESPACE Gem TARGETS Gem::ONNXRuntime.Editor.API)

endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ONNXRuntime.Tests
    if(PAL_TRAIT_ONNXRUNTIME_TEST_SUPPORTED)
        # We support ONNXRuntime.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME ONNXRuntime.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                onnxruntime_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::ONNXRuntime.Private.Object
        )

        # Add ONNXRuntime.Tests to googletest
        ly_add_googletest(
            NAME Gem::ONNXRuntime.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_ONNXRUNTIME_EDITOR_TEST_SUPPORTED)
            # We support ONNXRuntime.Editor.Tests on this platform, add ONNXRuntime.Editor.Tests target which depends on
            # private ONNXRuntime.Editor.Private.Object target
            ly_add_target(
                NAME ONNXRuntime.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    onnxruntime_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::ONNXRuntime.Private.Object
            )

            # Add ONNXRuntime.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::ONNXRuntime.Editor.Tests
            )
        endif()
    endif()
endif()
