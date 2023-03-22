#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
import subprocess
import unittest
import unittest.mock as mock

from o3de import utils


@pytest.mark.parametrize(
    "value, expected_result", [
        pytest.param('Game1', True),
        pytest.param('0Game1', False),
        pytest.param('the/Game1', False),
        pytest.param('', False),
        pytest.param('-test', False),
        pytest.param('test-', True),
    ]
)
def test_validate_identifier(value, expected_result):
    result = utils.validate_identifier(value)
    assert result == expected_result


@pytest.mark.parametrize(
    "value, expected_result", [
        pytest.param('{018427ae-cd08-4ff1-ad3b-9b95256c17ca}', False),
        pytest.param('', False),
        pytest.param('{018427aecd084ff1ad3b9b95256c17ca}', False),
        pytest.param('018427ae-cd08-4ff1-ad3b-9b95256c17ca', True),
        pytest.param('018427aecd084ff1ad3b9b95256c17ca', False),
        pytest.param('018427aecd084ff1ad3b9', False),
    ]
)
def test_validate_uuid4(value, expected_result):
    result = utils.validate_uuid4(value)
    assert result == expected_result


@pytest.mark.parametrize(
    "in_list, out_list", [
        pytest.param(['A', 'B', 'C'], ['A', 'B', 'C']),
        pytest.param(['A', 'B', 'C', 'A', 'C'], ['A', 'B', 'C']),
        pytest.param(['A', {'name': 'A', 'optional': True}], [{'name': 'A', 'optional': True}]),
        pytest.param([{'name': 'A', 'optional': True}, 'A'], ['A']),
        pytest.param([{'name': 'A'}], [{'name': 'A'}]),
        pytest.param([{'optional': False}], []),
    ]
)
def test_remove_gem_duplicates(in_list, out_list):
    result = utils.remove_gem_duplicates(in_list)
    assert result == out_list


#TODO: test CLICommand class


def test_prepend_file_to_system_path():
    import sys

    #check that a file's folder is in the path, not the file
    folder = pathlib.Path(__file__).parent
    utils.prepend_file_to_system_path(__file__)
    assert __file__ not in sys.path
    assert folder in sys.path

    sys.path.pop(0)
    assert folder not in sys.path

    #if a target folder is supplied instead of the file, that folder is prepended directly
    utils.prepend_file_to_system_path(folder)
    assert folder in sys.path

#Due to verifying project path setups, this is an integration test rather than a unit test
def test_get_project_path_from_file(tmp_path):
    
    TEST_PROJECT_JSON_PAYLOAD = '''
    {
        "project_name": "TestProject",
        "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
        "version" : "0.0.0",
        "compatible_engines" : [
            "o3de-sdk==2205.01"
        ],
        "engine_api_dependencies" : [
            "framework==1.2.3"
        ],
        "origin": "The primary repo for TestProject goes here: i.e. http://www.mydomain.com",
        "license": "What license TestProject uses goes here: i.e. https://opensource.org/licenses/MIT",
        "display_name": "TestProject",
        "summary": "A short description of TestProject.",
        "canonical_tags": [
            "Project"
        ],
        "user_tags": [
            "TestProject"
        ],
        "icon_path": "preview.png",
        "engine": "o3de-install",
        "restricted_name": "projects",
        "external_subdirectories": [
            "D:/TestGem"
        ]
    }
    '''

    #first we will check for a valid 
    project_path = tmp_path /"project_path"
    project_path.mkdir()

    project_json = project_path / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)

    test_folder = project_path / "nestedFolder"
    test_folder.mkdir()

    test_file = test_folder / "test.txt"
    test_file.write_text("THIS IS A TEST!")
    

    assert utils.get_project_path_from_file(test_file) == project_path

    assert utils.get_project_path_from_file(test_file, project_path) == project_path

    invalid_test_folder = tmp_path / 'invalid'
    invalid_test_folder.mkdir()

    invalid_test_file = invalid_test_folder / "invalid.txt"
    invalid_test_file.write_text("THIS IS INVALID!")

    assert utils.get_project_path_from_file(invalid_test_file) == None 

    assert utils.get_project_path_from_file(invalid_test_file, project_path) == project_path

    assert utils.get_project_path_from_file(invalid_test_file, invalid_test_folder) == None


#TODO: test load_and_execute_script
def test_load_and_execute_script(tmp_path):
    TEST_PYTHON_SCRIPT = """
import pathlib

folder = pathlib.Path(__file__).parent

with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write("This is a test")

    """
    test_folder = tmp_path / "test"
    test_folder.mkdir()
    test_script = test_folder / "test.py"

    test_script.write_text(TEST_PYTHON_SCRIPT)

    test_output = test_folder / "test_output.txt"

    assert not test_output.is_file()

    result = utils.load_and_execute_script(test_script)

    assert result == 0

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()

    assert text == "This is a test"



def test_load_and_execute_script_with_context(tmp_path):

    TEST_CONTEXT_PYTHON_SCRIPT = """
import pathlib

folder = pathlib.Path(__file__).parent

with open(folder / "test_output2.txt", 'w') as test_file:
    test_file.write(f"This is a test value: {context_value}")
    """

    test_folder = tmp_path / "test"
    test_folder.mkdir()

    test_script = test_folder / "test.py"

    test_script.write_text(TEST_CONTEXT_PYTHON_SCRIPT)

    test_output = test_folder / "test_output2.txt"

    assert not test_output.is_file()

    result= utils.load_and_execute_script(test_script, context_value=44)

    assert result == 0

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()
        
    assert text == "This is a test value: 44"

def test_load_and_execute_script_raises_exception_internally(tmp_path):
    TEST_ERR_PYTHON_SCRIPT = """
import pathlib

raise RuntimeError("Whoops")
print("hi there")
    """

    test_folder = tmp_path / "test"
    test_folder.mkdir()

    test_script = test_folder / "test.py"

    test_script.write_text(TEST_ERR_PYTHON_SCRIPT)

    result = utils.load_and_execute_script(test_script)

    assert result == 1


#TODO: test safe_kill_processes