#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import importlib.util
import logging
import os
import pathlib
import subprocess
from subprocess import Popen, PIPE
import sys

from o3de import manifest, utils
logger = logging.getLogger('o3de.export_project')
logging.basicConfig(format=utils.LOG_FORMAT)


class o3de_export_parameters(object):
    pass

#Helper API
def process_command(args: list,
                    cwd: pathlib.Path = None,
                    env: os._Environ = None,
                    show_logging: bool = True):
    process = Popen(args, cwd=cwd, env=env, stdout=PIPE, stderr=PIPE)

    #read any log lines coming from subprocess
    while process.poll() is None:
        line = process.stdout.readline()
        if not line: break

        log_line = line.decode('utf-8')
        if show_logging:
            print(log_line)
        
        logger.info(log_line)
    
    #flush out any remaining text from stdout
    log_lines = process.stdout.read().decode('utf-8')
    if show_logging:
        print(log_lines)
    
    stderr = process.stderr.read()

    #update this to safely kill a process
    process.kill()

    ret = process.returncode

    #if the process returns a FAILURE code (>0)
    if bool(ret):
        logger.error(stderr.decode('utf-8'))

    return ret



#Export Script entry  point
def _run_export_script(args: argparse, passthru_args: list) -> int:
    def validate_export_script(script_path):
        if not os.path.isfile(script_path):
            logger.error(f"The export script '{script_path}' does not exist!")
            return False

        if script_path.suffix != '.py':
            logger.error("A Python script with .py extension must be supplied for --export-script parameter!")
            return False
        return True
    
    def validate_project_path(project_path):
        if not os.path.isdir(project_path):
            logger.error(f"Project path is not a directory!")
            return False

        if not os.path.isfile(os.path.join(project_path, 'project.json')):
            logger.error("Project path is invalid: does not contain a project.json file!")
            return False
        return True


    export_script_path = args.export_script
    if not validate_export_script(export_script_path):
        print("Failed to process export script.")
        return 1
    
 
    project_path = args.project_path
    if project_path is not None:
        if not validate_project_path(project_path):
            print(f"Specified Project Path '{project_path}' is not valid.")
            return 1
    else:
        project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), export_script_path)
        if project_path is None:
            logger.error("Unable to find project folder associated with export script. Please specify using --project-path, or select an export script inside a project folder.")
            return 1

    script_name = os.path.basename(export_script_path)
    print(f"Begin loading export script '{script_name}'...")
    

    #Prepare import paths for export script ease of use
    #Allow for imports from O3DE CLI and the script's local directory 
    sys.path.insert(0, os.path.dirname(__file__))
    sys.path.insert(0, os.path.split(export_script_path)[0])

    #prepare O3DE arguments for script
    o3de_export = o3de_export_parameters()
    o3de_export.project_path = project_path
    o3de_export.engine_path = manifest.get_project_engine_path(project_path)
    o3de_export.args = passthru_args 


    #execute the script
    spec = importlib.util.spec_from_file_location(script_name, export_script_path)
    script_module = importlib.util.module_from_spec(spec)
    sys.modules[script_name] = script_module
    script_module.o3de_export = o3de_export
    spec.loader.exec_module(script_module)

    return 0




#Argument handling
def add_parser_args(parser):
    parser.add_argument('-es', '--export-script', type=pathlib.Path, required=True, help="An external Python script to run")
    parser.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                        help="Project to export. If not supplied, it will be inferred by the export script.")
    parser.set_defaults(func=_run_export_script)


def add_args(subparsers):
    """
    <TODO: INFO HERE>
    """
    export_subparser = subparsers.add_parser('export_project')
    add_parser_args(export_subparser)


def main():
    """
    Runs export_project.py as a standalone script
    """
    parser = argparse.ArgumentParser()
    add_parser_args(parser)
    args = parser.parse_known_args()
    ret = args.func(args) if hasattr(args, 'func') else 1
    sys.exit(ret)

if __name__ == "__main__":
    main()