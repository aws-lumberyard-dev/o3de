# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Tools/LyTestTools/tests
# Build directory: J:/Atom/lyfork/o3de/Tools/LyTestTools/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/debug" "J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/unit/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/LyTestTools_UnitTests.xml")
  set_tests_properties([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] PROPERTIES  LABELS "SUITE_smoke;COMPONENT_TestTools;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;12;ly_add_pytest;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/profile" "J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/unit/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/LyTestTools_UnitTests.xml")
  set_tests_properties([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] PROPERTIES  LABELS "SUITE_smoke;COMPONENT_TestTools;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;12;ly_add_pytest;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/release" "J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/unit/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/LyTestTools_UnitTests.xml")
  set_tests_properties([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] PROPERTIES  LABELS "SUITE_smoke;COMPONENT_TestTools;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "1500" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;147;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;316;ly_add_test;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;12;ly_add_pytest;J:/Atom/lyfork/o3de/Tools/LyTestTools/tests/CMakeLists.txt;0;")
else()
  add_test([=[LyTestTools_UnitTests.smoke::TEST_RUN]=] NOT_AVAILABLE)
endif()
