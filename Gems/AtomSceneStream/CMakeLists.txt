
#set(o3de_gem_path ${CMAKE_CURRENT_LIST_DIR})
#set(o3de_gem_json ${o3de_gem_path}/gem.json)
#o3de_read_json_key(o3de_gem_name ${o3de_gem_json} "AtomSceneStream")
#o3de_restricted_path(${o3de_gem_json} o3de_gem_restricted_path)

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${o3de_gem_restricted_path}" ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# project cmake for this platform.
include(${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_gem.cmake)

ly_add_external_target_path(${CMAKE_CURRENT_LIST_DIR}/3rdParty)

add_subdirectory(Code)

# ly_add_target(
#     NAME AtomSceneStream.Static STATIC
#     NAMESPACE Gem
#     FILES_CMAKE
#         AtomSceneStream_files.cmake
#     INCLUDE_DIRECTORIES
#         PRIVATE
#             Code
#         PUBLIC
#             Code/Include
#     BUILD_DEPENDENCIES
#         PRIVATE
#             AZ::AzCore
#             Gem::LmbrCentral
#             Gem::Atom_RHI.Public
#             Gem::Atom_RPI.Public
#             Gem::Atom_Feature_Common.Static
#             Gem::AtomLyIntegration_CommonFeatures.Static
# )
# 
# ly_add_target(
#     NAME AtomSceneStream ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
#     NAMESPACE Gem
#     FILES_CMAKE
#         AtomSceneStream_shared_files.cmake
#     INCLUDE_DIRECTORIES
#         PRIVATE
#             Code/AssetPipeline
#             Code
#         PUBLIC
#             Code/Include
#     BUILD_DEPENDENCIES
#         PRIVATE
#             AZ::AzCore
#             Gem::Atom_Utils.Static
#             Gem::AtomSceneStream.Static
# )