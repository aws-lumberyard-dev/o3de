# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Gems/NvCloth/Code
# Build directory: J:/Atom/lyfork/o3de/External/NvCloth-07f91111/Code
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::NvCloth.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/NvCloth.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;132;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::NvCloth.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/NvCloth.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;132;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::NvCloth.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/NvCloth.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;132;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
else()
  add_test([=[Gem::NvCloth.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/NvCloth.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;162;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/NvCloth.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;162;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/NvCloth.Editor.Tests.Gem.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_NvCloth.Editor.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;162;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/NvCloth/Code/CMakeLists.txt;0;")
else()
  add_test([=[Gem::NvCloth.Editor.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
