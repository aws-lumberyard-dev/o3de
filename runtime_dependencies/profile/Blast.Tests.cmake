#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

cmake_policy(SET CMP0012 NEW) # new policy for the if that evaluates a boolean out of "if(NOT ${same_location})"

function(ly_copy source_file target_directory)
    get_filename_component(target_filename "${source_file}" NAME)
    cmake_path(COMPARE "${source_file}" EQUAL "${target_directory}/${target_filename}" same_location)
    if(NOT ${same_location})
        file(LOCK ${target_directory}/${target_filename}.lock GUARD FUNCTION TIMEOUT 300)
        if("${source_file}" IS_NEWER_THAN "${target_directory}/${target_filename}")
            message(STATUS "Copying \"${source_file}\" to \"${target_directory}\"...")
            file(COPY "${source_file}" DESTINATION "${target_directory}" FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE FOLLOW_SYMLINK_CHAIN)
            file(TOUCH_NOCREATE ${target_directory}/${target_filename})
        endif()
    endif()    
endfunction()

ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlast_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtAssetUtils_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtAuthoring_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtExporter_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtPhysX_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtPxSerialization_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtSerialization_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtShaders_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtStress_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastExtTkSerialization_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastGlobals_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/Blast-v1.1.7_rc2-9-geb169fe-rev2-windows/Blast/bin/vc15win64-cmake/profile/NvBlastTk_x64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//profile/bin/PhysX_64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//profile/bin/PhysXCooking_64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//profile/bin/PhysXFoundation_64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared//profile/bin/PhysXCommon_64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared/profile/bin/PhysXDevice64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/PhysX-4.1.2.29882248-rev3-windows/PhysX/pxshared/profile/bin/PhysXGpu_64.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/boost_filesystem-vc141-mt-x64-1_73.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/boost_thread-vc141-mt-x64-1_73.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Half-2_5.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/heif.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Iex-2_5.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/IlmImf-2_5.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/IlmThread-2_5.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/Imath-2_5.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/jpeg62.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/lzma.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/libpng16.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/tiff.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/zlib1.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/yaml-cpp.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenColorIO.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO.pyd" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/OpenImageIO_Util.dll" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/iconvert.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/idiff.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/igrep.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/iinfo.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/maketx.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("C:/Users/luissemp/.o3de/3rdParty/packages/openimageio-2.1.16.0-rev2-windows/OpenImageIO/2.1.16.0/win_x64/bin/oiiotool.exe" "J:/Atom/lyfork/o3de/bin/profile")
ly_copy("J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile")


file(TOUCH J:/Atom/lyfork/o3de/runtime_dependencies/profile/Blast.Tests_profile.stamp)
