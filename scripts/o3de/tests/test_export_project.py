import argparse
import pytest
import pathlib
from o3de import utils, manifest
from o3de.export_project import O3DEScriptExportContext, execute_python_script, add_args, _run_export_script

TEST_PYTHON_SCRIPT = """
import pathlib

folder = pathlib.Path(__file__).parent

with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write(f"This is a test for the following: {o3de_context.args[0]}")

    """

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

#TODO: test process_command

def test_execute_python_command(tmp_path):
    import sys

    test_folder = tmp_path / "test"
    test_folder.mkdir()
    test_script = test_folder / "test.py"

    test_script.write_text(TEST_PYTHON_SCRIPT)

    test_output = test_folder / "test_output.txt"

    assert not test_output.is_file()

    project_path = tmp_path /"project_path"
    project_path.mkdir()

    project_json = project_path / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    o3de_context = O3DEScriptExportContext(export_script_path= test_script,
                                        project_path = project_path,
                                        engine_path = manifest.get_project_engine_path(project_path),
                                        args = ['456'])

    result = execute_python_script(test_script, o3de_context)

    assert result == 0

    assert test_folder in sys.path

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()
    
    assert text == "This is a test for the following: 456"

    o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"
    assert o3de_cli_folder in sys.path



def test_run_export_script(tmp_path):
    import sys

    test_folder = tmp_path / "test_project"
    test_folder.mkdir()
    test_script_folder = test_folder / "ExportScripts"
    test_script_folder.mkdir()

    project_json = test_folder / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    test_script = test_script_folder / "test.py"
    test_script.write_text(TEST_PYTHON_SCRIPT)

    test_output = test_script_folder / "test_output.txt"

    assert not test_output.is_file()

    the_parser = argparse.ArgumentParser()
    subparsers = the_parser.add_subparsers()
    add_args(subparsers)
    
    known_args, unknown_args = the_parser.parse_known_args([
        'export-project',
        '-es', str(test_script),
        '-ll', 'INFO',
        '456'
    ])

    assert hasattr(known_args, 'accepts_partial_args')

    result = _run_export_script(known_args, unknown_args)

    assert result == 0

    assert test_script_folder in sys.path

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()
    
    assert text == "This is a test for the following: 456"

    o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"
    assert o3de_cli_folder in sys.path


def test_run_export_script_with_separate_project(tmp_path):
    import sys

    project_folder = tmp_path / "test_project"
    project_folder.mkdir()

    project_json = project_folder / "project.json"
    project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)


    script_folder = tmp_path / "export_scripts"
    script_folder.mkdir()

    test_script = script_folder / "test.py"
    test_script.write_text(TEST_PYTHON_SCRIPT)

    test_output = script_folder / "test_output.txt"

    assert not test_output.is_file()

    the_parser = argparse.ArgumentParser()
    subparsers = the_parser.add_subparsers()
    add_args(subparsers)
    
    known_args, unknown_args = the_parser.parse_known_args([
        'export-project',
        '-es', str(test_script),
        '-pp', str(project_folder),
        '-ll', 'INFO',
        '456'
    ])

    assert hasattr(known_args, 'accepts_partial_args')

    result = _run_export_script(known_args, unknown_args)

    assert result == 0

    assert script_folder in sys.path

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()
    
    assert text == "This is a test for the following: 456"

    o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"
    assert o3de_cli_folder in sys.path