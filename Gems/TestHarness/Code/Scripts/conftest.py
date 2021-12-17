"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Hooks for functions in the pytest package. This files is required to be named conftest.py,
per specifications of the pytest package.
"""

import pytest

def pytest_addoption(parser):
    """
    Adds command line options to pytest to allow settings configuration by users.
    :param parser: pytest-provided fixture for argparse.ArgumentParser.
    """
    parser.addoption("--testharness_platform", action="store", default="win_x64_vs2015")
    parser.addoption("--testharness_configuration", action="store", default="profile")
    parser.addoption("--tag", action="store")

@pytest.hookimpl(hookwrapper=True, tryfirst=True)
def pytest_collection_modifyitems(items):
    """
        Modifies the list of tests to run prior to test execution, with code preceding 
        the keyword "yield" being executed before the pytest_collection_modifyitems() 
        function defined in the pytest package is run.

        The primary reason that the test_enabled_status is checked here rather than
        in test_fn() in TestHarnessLauncher.py is in light of the fact that future
        work on handling tags will occur here at collection time, and it seemed
        reasonable to centralize the checks at test collection time where possible, 
        with lower preference of checks at test execution time.

        Note that if all tests are disabled in the Editor, they are skipped and
        therefore not executed, and so testharness_execution.json won't contain
        these skipped tests' information, because testharness_execution.json documents
        only tests that are meant to be executed.

        :param items: list of pytest test_x functions that have been collected.
    """
    for item in items:
        # item.name is string type, ex. "test_fn[win_x64_vs2015-profile-SamplesProject-all-TestHarnessDemo-SanityTest-True-TestHarnessDemo,SamplesProject]"
        # An assumption on the order of test parameters is made here based on the string item.name
        test_params = item.name.split('[')[1].split(']')[0].split('-')
        test_platform = test_params[0]
        test_configuration = test_params[1]
        test_project = test_params[2]
        test_specification = test_params[3]
        test_level = test_params[4]
        test_name = test_params[5]
        test_enabled_status = test_params[6]
        test_tags = test_params[7].split(',') # commas may delineate tags, spaces won't be part of tags because of how spaces are treated at CLI.
        input_tags = pytest.config.getoption("tag")
        if input_tags != "None":
            input_tags = input_tags.split(',')
            skipFlag = True
            for tag in input_tags:
                if tag in test_tags:
                    skipFlag = False
            if skipFlag == True: 
                skip = pytest.mark.skip(reason="User tag from CLI filtered the test out.")
                item.add_marker(skip)
        if test_enabled_status == "False":
            skip = pytest.mark.skip(reason="Test component was marked disabled in the level.")
            item.add_marker(skip)
    yield