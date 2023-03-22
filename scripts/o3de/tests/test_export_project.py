import pytest
import pathlib
from o3de import utils, manifest
from o3de.export_project import O3DEScriptExportContext, execute_python_script

#TODO: test process_command

def test_execute_python_command(tmp_path):
    import sys

    TEST_PYTHON_SCRIPT = """
import pathlib

folder = pathlib.Path(__file__).parent

with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write(f"This is a test for the following: {o3de_context.test_value}")

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
                                        args = ['this', 'is', 'a', 'test'])
    o3de_context.test_value = "456"

    result = execute_python_script(test_script, o3de_context)

    assert result == 0

    assert test_folder in sys.path

    assert test_output.is_file()

    with open(test_output, 'r') as t_out:
        text = t_out.read()
    
    assert text == "This is a test for the following: 456"

    o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"
    print(o3de_cli_folder)
    assert o3de_cli_folder in sys.path


#TODO: test _run_export_script