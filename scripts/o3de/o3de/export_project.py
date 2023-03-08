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
from o3de.validation import *

#grab process utilities from Tools/LyTestTools
sys.path.insert(0, os.path.join(os.path.dirname(__file__),"..", "..","..","Tools","LyTestTools","ly_test_tools","environment"))
import process_utils as putil

logger = logging.getLogger(__name__)
logging.basicConfig(format=utils.LOG_FORMAT)

READ_ONLY_PARAMS = ["export_script", "project_path", "engine_path", "logger", "args"]
LOGGER_PARAM = "logger"

# a singleton which manages all parameters for the duration of the export process
# the export script and any script it calls via execute_python_script have access to these params 
class O3DEScriptExportParameters(utils.BaseSingleton):
    _attributes = {}
    _read_only_attribute_names = set()

    def __setattr__(self, attr, value):
        if attr in O3DEScriptExportParameters._read_only_attribute_names:
            logger.error(f"Cannot set '{attr}' to '{value}', it is a read-only parameter!")
        else:
            O3DEScriptExportParameters._attributes[attr] = value
    def __getattr__(self, attr):
        if attr in O3DEScriptExportParameters._attributes: 
            return O3DEScriptExportParameters._attributes[attr]
        else:
            logger.error(f"Attribute '{attr}' does not exist in export parameters!")
            return None
    def as_dict(self):
        return O3DEScriptExportParameters._attributes


#Helper API
def mark_readonly_params(*params:str):
    for p in params:
        O3DEScriptExportParameters._read_only_attribute_names.add(p)

def mark_writeable_params(*params:str):
    for p in params:
        if p in O3DEScriptExportParameters._read_only_attribute_names:
            O3DEScriptExportParameters._read_only_attribute_names.remove(p)

def process_command(args: list,
                    cwd: pathlib.Path = None,
                    env: os._Environ = None,
                    show_logging: bool = True) -> int:
    
    logger = O3DEScriptExportParameters().logger
    
    def poll_process(process):
        #read any log lines coming from subprocess
        while process.poll() is None:
            line = process.stdout.readline()
            if not line: break
            log_line = line.decode('utf-8')
            logger.info(log_line)
    
    def cleanup_process(process) -> str:
        #flush remaining log lines
        log_lines = process.stdout.read().decode('utf-8')
        logger.info(log_lines)
        stderr = process.stderr.read()

        logger.info("Finishing current command...")
        putil._safe_kill_processes([process])
        
        return stderr

    try:
        with Popen(args, cwd=cwd, env=env, stdout=PIPE, stderr=PIPE) as process:
            poll_process(process)
            
            stderr = cleanup_process(process)

            ret = process.returncode            
            if stderr:
                # bool(ret) --> if the process returns a FAILURE code (>0)
                logger_func = logger.error if bool(ret) else logger.warning
                logger_func(stderr.decode('utf-8'))
    except RuntimeError as re:
        logger.error(re)
        raise re
        

    return ret


def execute_python_script(target_script_path: pathlib.Path or str, override_logger=False, **kwargs) -> int:
    if type(target_script_path) == str:
        target_script_path = pathlib.Path(target_script_path)

    script_name = os.path.basename(target_script_path)
    logger.info(f"Begin loading script '{script_name}'...")

    #Prepare import paths for export script ease of use
    #Allow for imports from calling script and the target script's local directory

    current_folder_path = os.path.dirname(__file__)
    if current_folder_path not in sys.path: 
        sys.path.insert(0, current_folder_path)

    target_folder_path = os.path.split(target_script_path)[0]    
    if target_folder_path not in sys.path:
        sys.path.insert(0, target_folder_path)


    spec = importlib.util.spec_from_file_location(script_name, target_script_path)
    script_module = importlib.util.module_from_spec(spec)
    sys.modules[script_name] = script_module

    o3de_export = O3DEScriptExportParameters()
    for key,value in kwargs.items():
        setattr(o3de_export, key, value)

    if override_logger:
        parent_logger = o3de_export.logger
        mark_writeable_params(LOGGER_PARAM)
        o3de_export.logger = parent_logger.getChild(script_name.replace(".py", ""))
        mark_readonly_params(LOGGER_PARAM)

    try:
        spec.loader.exec_module(script_module)
    except RuntimeError as re:
        logger.error(f"Failed to run script '{target_script_path}': \nException: "+ str(re))
        return 1
    finally:
        if override_logger:
            mark_writeable_params(LOGGER_PARAM)
            o3de_export.logger = parent_logger
            mark_readonly_params(LOGGER_PARAM)

    return 0

#Export Script entry  point
def _run_export_script(args: argparse, passthru_args: list) -> int:
    def get_project_path(args):
        project_path = args.project_path
        if not project_path:
            project_path = utils.find_ancestor_dir_containing_file(pathlib.PurePath('project.json'), export_script_path)
            if not project_path:
                logger.error("Unable to find project folder associated with export script. Please specify using --project-path, or select an export script inside a project folder.")
                return None   
        
        if not valid_o3de_project_path(project_path):
            logger.error(f"Specified Project Path '{project_path}' is not valid.")
            return None
        return project_path
        
   
    logger.setLevel(args.log_level)

    export_script_path = args.export_script
    if not valid_export_script(export_script_path):
        logger.error("Failed to process export script.")
        return 1
 
    project_path = get_project_path(args)
    if not project_path:
        return 1
    
    #prepare O3DE arguments for script
    o3de_export = O3DEScriptExportParameters()
    
    o3de_export.project_path = project_path
    o3de_export.engine_path = manifest.get_project_engine_path(project_path)
    o3de_export.logger = logger
    o3de_export.args = passthru_args 

    mark_readonly_params(*READ_ONLY_PARAMS)

    ret = execute_python_script(export_script_path)

    logger.info("Finished exporting")
    return ret


#Argument handling
def add_parser_args(parser):
    parser.add_argument('-es', '--export-script', type=pathlib.Path, required=True, help="An external Python script to run")
    parser.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                        help="Project to export. If not supplied, it will be inferred by the export script.")
    
    parser.add_argument('-v', '--verbosity', dest='log_level', default='ERROR',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the log verbosity level")
    
    parser.set_defaults(func=_run_export_script, accepts_partial_args=True)
    

def add_args(subparsers):
    export_subparser = subparsers.add_parser('export-project')
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
