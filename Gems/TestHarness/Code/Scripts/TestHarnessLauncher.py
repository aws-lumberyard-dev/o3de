"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Retrieves a list of registered tests from testharness_collection.json, creates a list of
parameter sets to try, repeatedly launches the launcher to run the tests, writes test
runner information in testharness_execution.json, and stores test reports in xml and html 
format.

Current usage from dev\:
$lmbr_test testharness [optional flags and options]

Assumption:
There exists a file testharness_collection.json generated prior to test collection, whether
by hand or when a client saves a level in the Editor (i.e. CTRL-S in the Editor). The file 
will contain all registered tests to run and associated information (level, project, entity 
ID, is enabled or not).

Current limitations:
Clients may be interested in running a set of tests on various platform and configuration 
settings, but currently only singular platform and configuration settings are supported 
(can only execute tests of one platform and one configuration). 
"""


import pytest
import os
import time
import json
import utils 
import itertools

from test_tools import WINDOWS_LAUNCHER
import test_tools.builtin.fixtures as fixtures
import test_tools.shared.waiter as waiter
import test_tools.builtin.helpers as helpers
import test_tools.launchers.launcher as launcher_nonfixtures
from test_tools.shared.remote_console_commands import RemoteConsole

# activate the builtin workspace and launcher fixture
workspace = fixtures.use_fixture(fixtures.builtin_workspace_fixture, scope='function')
launcher = fixtures.use_fixture(fixtures.launcher, scope='function')

# fixture for creating a remote console instance
@pytest.fixture
def remote_console(request):
    remote_console_instance = RemoteConsole()
    def teardown():
        if remote_console_instance.connected:
            remote_console_instance.stop()
    request.addfinalizer(teardown)
    return remote_console_instance

def get_registered_test_information():
    """
    Parses testharness_collection.json to retrive a list of registered tests' information,
    including each test's name, corresponding level name and project name, and is enabled
    status.

    If testharness_execution.json can't be found, an IO error will be thrown indicating 
    file not found.

    In cases where project name, level name, or test case entity is not found, the pytest 
    test will be marked as failed.

    All tests in testharness_collection.json will be collected and fed into pytest here.
    Modification to the list of tests to be executed (i.e. tests marked disabled in the
    Editor, tests filtered from client input) is done in a hook on pytest_collection_modifyitems() 
    in conftest.py. 

    In cases where testharness_collection.json is modified such that duplicate keys exist,
    the last key of any key collisions will be chosen while others are ignored, due
    to the usage of Python dictionaries. An implication is that tests in the same 
    level of the same project with the same test names, which are keys, won't be run.

    It is safe to assume that project, level and test name, which are all keys, are
    strings, per JSON format requirements.

    :return: list of tuples storing registered test names (type: unicode), level_names 
    (type: unicode), project_names (type: unicode), is enabled status (type: bool)
    """
    test_information_list = []
    with open(utils.FILE_PATH) as json_log:
        data = json.load(json_log)
    try:
        for project_name, level_data in data.items():
            for level_name, test_data in level_data.items():
                for test_name, test_attributes in test_data.items():
                    test_enabled_status = test_attributes["is_enabled"]
                    assert(isinstance(test_enabled_status, bool))
                    tags = test_attributes["tags"]
                    test_information = (test_name, level_name, project_name, test_enabled_status, tags)
                    test_information_list.append(test_information)
        return test_information_list
    except:
        print("Unexpected error while parsing testharness_collection.json.")
        raise
    

@pytest.mark.parametrize("platform, configuration, project, spec, level, test_name, test_enabled_status, tags", [
    pytest.param(pytest.config.getoption("testharness_platform"), # Note: should be automatically determined in the future
                 pytest.config.getoption("testharness_configuration"), # Note: should be automatically determined in the future
                 project_name, 
                 utils.SPEC,
                 level_name,
                 test_name,
                 test_enabled_status,
                 tags) 
                 for (test_name, level_name, project_name, test_enabled_status, tags) in get_registered_test_information()
])
class TestHarnessTests(object):
    def test_fn(self, request, launcher, remote_console, test_name, test_enabled_status, tags):
        """
        The boilerplate function that the pytest package will find and run on each
        provided set of parameters consecutively. For each set of parameters, a 
        launcher and a remote console instance will be started and a test written 
        in C++, Lua, or ScriptCanvas will be run.

        :param request: pytest request object
        :param launcher: a fixture of a launcher for a workspace and level specified 
        in parametrize
        :param remote_console: a fixture of a remote console
        :param test_name: the name of the test to run (type: unicode)
        :param test_enabled_status: whether the test is marked as enabled or not 
        in the level (type: bool). This is already checked prior to the invocation
        of test_fn() at collection time in a hook on pytest_collection_modifyitems() 
        in conftest.py but must be passed in, per Pytest parametrize requirements.
        :param tags: user-defined tags and static tags (i.e. a test's associated
        level name, project name) (type: unicode)
        """
        try:
            utils.start_launcher(launcher)
            remote_console.start()
            file_path = utils.send_ready_request(remote_console, launcher)
            run_test(remote_console, file_path, test_name)
        finally:
            utils.launcher_teardown(launcher)
            utils.remote_console_teardown(remote_console)

def run_test(remote_console, file_path, test_name, test_number=1):
    """
    Runs a specified test given by the test_name.
    
    :param remote_console: the currently running remote console
    :param file_path: path to testharness_execution.json (type: string)
    :param test_name: name of the test entity to run (type: string)
    :param test_number: number of tests we're on in the same launcher instance (type: int)
    """
    node_name = 'Test-{}_{}'.format(test_number, test_name)
    command = 'th_RunTest {}'.format(test_name)
    remote_console.send_command(command)
    request_time = int(round(time.time() * 1000)) # in milliseconds
    waiter.wait_for(lambda: os.path.isfile(file_path) and
                                            check_test_status_format(request_time, file_path, node_name),
                                            exc=AssertionError("Engine never gave valid test result."))
    check_if_test_passed(file_path, node_name)   

def check_test_status_format(request_time, file_path, node_name):
    """
    Checks if the test result reported from C++ game runtime in 
    testharness_execution.json is formatted correctly.

    The try/except when opening a json file handles the rare ValueError 
    case of this function trying to load in testharness_execution.json 
    as it is currently being written by the TestHarnessSystemComponent.

    :param request_time: the time in seconds (type: float)
    :param file_path: path to testharness_execution.json (type: string)
    :param node_name: name of current node (type: string)

    :return boolean: True if the test result timestamp exists, if 
    test result time is after the test request time, and if there is
    the "passed" key in testharness_execution.json.
    """
    try:
        with open(file_path) as json_log:
            data = json.load(json_log)
    except ValueError:
        return False
    if not node_name in data:
        return False
    try:
        test_result_time = data[node_name]["result_timestamp"]
    except ValueError:
        return False
    if request_time > test_result_time:
        return False
    if "pass" in data[node_name]:
        return True
    return False

def check_if_test_passed(file_path, node_name):
    """
    Checks testharness_execution.json to see if a test has passed.

    If testharness_execution.json can't be found, an IO error will
    be thrown indicating file not found.

    If a test fails because the test entity was not found in the level
    in C++ game runtime, because the test timed out, or because of 
    an assertion, an assertion error will be raised, marking the test as
    having failed.

    :param file_path: path to testharness_execution.json (type: string)
    :param node_name: name of current node (type: string)
    """
    error = "The test failed because "
    failure_reason = ""
    with open(file_path) as json_log:
        data = json.load(json_log)
    if not data[node_name]["pass"]:
        if data[node_name]["missing"]:
            failure_reason = "it was not found in the level."
        elif data[node_name]["timeout"]:
            failure_reason = "it timed out."
        else:
            failure_reason = "it failed at least one expect true while running."
        raise AssertionError(error + failure_reason)