# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Code/Framework/AzCore
# Build directory: J:/Atom/lyfork/o3de/Code/Framework/AzCore
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;130;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;130;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/AzCore.Tests.dll" "AzRunUnitTests" "--gtest_output=xml:J:/Atom/lyfork/o3de/Testing/Gtest/AZ_AzCore.Tests.xml" "--gtest_filter=-*SUITE_smoke*:*SUITE_periodic*:*SUITE_benchmark*:*SUITE_sandbox*")
  set_tests_properties([=[AZ::AzCore.Tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_googletest" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;407;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;130;ly_add_googletest;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzCore.Tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/debug/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/debug/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;133;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/profile/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/profile/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;133;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] "J:/Atom/lyfork/o3de/bin/release/AzTestRunner.exe" "J:/Atom/lyfork/o3de/bin/release/AzCore.Tests.dll" "AzRunBenchmarks" "--benchmark_out_format=json" "--benchmark_out=J:/Atom/lyfork/o3de/BenchmarkResults/AzCore.Benchmarks.json")
  set_tests_properties([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] PROPERTIES  LABELS "SUITE_benchmark;FRAMEWORK_googlebenchmark" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;497;ly_add_test;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;133;ly_add_googlebenchmark;J:/Atom/lyfork/o3de/Code/Framework/AzCore/CMakeLists.txt;0;")
else()
  add_test([=[AZ::AzCore.Benchmarks.benchmark::TEST_RUN]=] NOT_AVAILABLE)
endif()
