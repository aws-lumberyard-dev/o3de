# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Code/Tools/AssetBundler
# Build directory: J:/Atom/lyfork/o3de/Code/Tools/AssetBundler
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AssetBundler.Tests.exe" "--unittest" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AssetBundler.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;93;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AssetBundler.Tests.exe" "--unittest" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AssetBundler.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;93;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AssetBundler.Tests.exe" "--unittest" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AssetBundler.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;93;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Tools/AssetBundler/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AssetBundler.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
