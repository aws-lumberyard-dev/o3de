#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
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

#TODO: test load_and_execute_script

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

#TODO: test get_project_path_from_file

#TODO: test safe_kill_processes