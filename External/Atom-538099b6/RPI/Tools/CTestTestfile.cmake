# CMake generated Testfile for 
# Source directory: J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools
# Build directory: J:/Atom/lyfork/o3de/External/Atom-538099b6/RPI/Tools
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/debug" "J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;313;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Pp][Rr][Oo][Ff][Ii][Ll][Ee])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/profile" "J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;313;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] "J:/Atom/lyfork/o3de/python/python.cmd" "-s" "-B" "-m" "pytest" "-v" "--tb=short" "--show-capture=log" "-c" "J:/Atom/lyfork/o3de/ctest_pytest.ini" "--build-directory" "J:/Atom/lyfork/o3de/bin/release" "J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/atom_rpi_tools/tests/" "--junitxml=J:/Atom/lyfork/o3de/Testing/Pytest/RPI_atom_rpi_tools_tests.xml")
  set_tests_properties([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] PROPERTIES  LABELS "SUITE_main;FRAMEWORK_pytest" RUN_SERIAL "FALSE" TIMEOUT "30" _BACKTRACE_TRIPLES "J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;144;add_test;J:/Atom/lyfork/o3de/cmake/LYTestWrappers.cmake;313;ly_add_test;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;13;ly_add_pytest;J:/Atom/lyfork/o3de/Gems/Atom/RPI/Tools/CMakeLists.txt;0;")
else()
  add_test([=[RPI::atom_rpi_tools_tests.main::TEST_RUN]=] NOT_AVAILABLE)
endif()
