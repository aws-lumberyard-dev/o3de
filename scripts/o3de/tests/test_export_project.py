import argparse
import pytest
import pathlib
import unittest.mock as mock
from unittest.mock import patch
from o3de import utils, manifest
from o3de.export_project import O3DEScriptExportContext, execute_python_script, add_args, _export_script, process_command

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

#Note: the underlying command logic is found in CLICommand class object. That is tested in test_utils.py
@pytest.mark.parametrize("args, expected_result",[
    pytest.param(["cmake", "--version"], 0),
    pytest.param(["cmake"], 0),
    pytest.param(["cmake", "-B"], 1),
    pytest.param([], 1),
])
def test_process_command(args, expected_result):

    cli_command = mock.Mock()
    cli_command.run.return_value = expected_result

    with patch("o3de.utils.CLICommand", return_value=cli_command) as cli:
        result = process_command(args)
        assert result == expected_result



#The following functions will do integration tests of _export_script and execute_python_script, thereby testing all of script execution
TEST_PYTHON_SCRIPT = """
import pathlib

folder = pathlib.Path(__file__).parent

with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write(f"This is a test for the following: {o3de_context.args[0]}")

    """

@pytest.mark.parametrize("args, should_pass_project_folder, project_folder_subpath, script_folder_subpath, output_filename, expected_result", [
    #successful cases
    pytest.param(['456'], False, ["test_project"], ["test_project" , "ExportScripts"], "test_output.txt", 0),
    pytest.param(['456'], True, ["test_project"], ["test_project" , "ExportScripts"], "test_output.txt", 0),
    pytest.param(['456'], True, ["test_project"], ["export_scripts"], "test_output.txt", 0),
    pytest.param([456], True, ["test_project"], ["export_scripts"], "test_output.txt", 0),

    #failure cases
    pytest.param([], True, ["test_project"], ["export_scripts"], "test_output.txt", 1),
    pytest.param([456], False, ["test_project"], ["export_scripts"], "test_output.txt", 1),
])
def test_export_script(tmp_path, args, should_pass_project_folder, project_folder_subpath, script_folder_subpath, output_filename, expected_result):
    import sys

    project_folder = tmp_path 
    for pf in project_folder_subpath:
        project_folder = project_folder / pf
    project_folder.mkdir()

    script_folder = tmp_path 
    for sf in script_folder_subpath:
        script_folder = script_folder / sf
    script_folder.mkdir()


    project_json = project_folder / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    test_script = script_folder / "test.py"
    test_script.write_text(TEST_PYTHON_SCRIPT)

    test_output = script_folder / output_filename

    assert not test_output.is_file()
    
    result = _export_script(test_script, project_folder if should_pass_project_folder else None, args)

    assert result == expected_result


    #only check for these if we're simulating a successful case
    if result == 0:
        assert script_folder in sys.path

        assert test_output.is_file()

        with open(test_output, 'r') as t_out:
            text = t_out.read()
        
        if len(args) > 0:
            assert text == f"This is a test for the following: {args[0]}"

        o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"
        assert o3de_cli_folder in sys.path
    

TEST_ERR_PYTHON_SCRIPT = """
import pathlib

raise RuntimeError("Whoops")
print("hi there")
    """

def test_export_script_runtime_error(tmp_path):
    project_folder = tmp_path / "test_project"
    project_folder.mkdir()

    script_folder = project_folder / "export_scripts"
    script_folder.mkdir()

    project_json = project_folder / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    test_script = script_folder / "test.py"
    test_script.write_text(TEST_ERR_PYTHON_SCRIPT)
    
    result = _export_script(test_script, project_folder, [])

    assert result == 1

