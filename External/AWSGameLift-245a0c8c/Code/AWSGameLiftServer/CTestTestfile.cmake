# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer
# Build directory: J:/Atom/lyfork/o3de/External/AWSGameLift-245a0c8c/Code/AWSGameLiftServer
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/AWSGameLift.Server.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_AWSGameLift.Server.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;68;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/AWSGameLift.Server.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_AWSGameLift.Server.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;68;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/AWSGameLift.Server.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/Gem_AWSGameLift.Server.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*:*SUITE_awsi*")
  set_tests_properties([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;410;ly_add_test;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;68;ly_add_googletest;J:/Atom/lyfork/o3de/Gems/AWSGameLift/Code/AWSGameLiftServer/CMakeLists.txt;0;")
else()
  add_test([=[Gem::AWSGameLift.Server.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
