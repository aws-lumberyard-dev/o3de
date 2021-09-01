# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework
# Build directory: J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;91;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;91;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/AzToolsFramework.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzToolsFramework.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;91;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzToolsFramework.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;94;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;94;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/AzToolsFramework.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzToolsFramework.Benchmarks.json")
  set_tests_properties([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;94;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzToolsFramework/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzToolsFramework.Benchmarks.benchmark::TEST_RUN]=] NOT_AVAILABLE)
endif()
