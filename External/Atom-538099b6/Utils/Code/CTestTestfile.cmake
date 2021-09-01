# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code
# Build directory: J:/Atom/lyfork/o3de/External/Atom-538099b6/Utils/Code
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/Atom_Utils.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_Atom_Utils.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;48;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/Atom_Utils.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_Atom_Utils.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;48;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/Atom_Utils.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_Atom_Utils.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;48;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/Atom/Utils/Code/CMakeLists.txt;0;")
else()
  add_test([=[Gem::Atom_Utils.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
