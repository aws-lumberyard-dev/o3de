#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import argparse
import logging
import os
import pathlib
import sys

from o3de import manifest, utils
from o3de.validation import validate_export_script

logger = logging.getLogger(__name__)
logging.basicConfig(format=utils.LOG_FORMAT)

class O3DEScriptExportContext(object):
    """
    The context object is used to store parameter values and variables throughout the lifetime of an export script's execution.
    It can also be passed onto nested scripts the export script may execute, which can in turn update the context as necessary.
    """
    
    def __init__(self, export_script_path: pathlib.Path,
                       project_path: pathlib.Path,
                       engine_path: pathlib.Path,
                       logger: logging.Logger,
                       args: list = []) -> None:
        object.__setattr__(self,"_read_only_names", set())
        self._export_script_path = export_script_path
        self._project_path = project_path
        self._engine_path = engine_path
        self._logger = logger
        self._args = args
        object.__setattr__(self, "_read_only_names", set(["_export_script_path", "export_script_path",
                                                          "_project_path", "project_path",
                                                          "_engine_path", "engine_path",
                                                          "_logger", "logger",
                                                          "_args", "args"]))

    @property
    def export_script_path(self) -> pathlib.Path:
        """The absolute path to the export script being run."""
        return self._export_script_path
    
    @property
    def project_path(self) -> pathlib.Path:
        """The absolute path to the project being exported."""
        return self._project_path
    
    @property
    def engine_path(self) -> pathlib.Path:
        """The absolute path to the engine that the project is built with."""
        return self._engine_path
    
    @property
    def logger(self) -> logging.Logger:
        """Instance of the logger for export_project.py that export scripts can use for consistent logging."""
        return self._logger
    
    @property
    def args(self) -> list:
        """A list of the CLI arguments that were unparsed, and passed through for further processing, if necessary."""
        return self._args
    

    def __setattr__(self, attr, value) -> None:
        if attr in self._read_only_names:
            logger.error(f"Cannot set '{attr}' to '{value}', it is a read-only parameter!")
        else:
            self.__dict__[attr] = value
    def __getattr__(self, attr) -> None:
        if attr in self.__dict__: 
            return self.__dict__[attr]
        else:
            logger.error(f"Attribute '{attr}' does not exist in export parameters!")
            return None

    def mark_readonly(self, *params:str) -> None:
        """Provide the string names of context parameters/variables that should be read only. """
        for p in params:
            self._read_only_names.add(p)

    def mark_writeable(self, *params:str) -> None:
        """Provide the string names of context parameters/variables that should be writeable. """
        for p in params:
            if p in self._read_only_names:
                self._read_only_names.remove(p)
    
    def set_params(self, **kwargs) -> None:
        for k, v in kwargs.items():
            setattr(self, k, v)

    def as_dict(self) -> dict:
        return self.__dict__


#Helper API
def process_command(args: list,
                    cwd: pathlib.Path = None,
                    env: os._Environ = None) -> int:
    """
    Wrapper for subprocess.Popen, which handles polling the process for logs, reacting to failure, and cleaning up the process.
    :param args: A list of space separated strings which build up the entire command to run. Similar to the command list of subprocess.Popen
    :param cwd: The desired current working directory of the command. Useful for commands which require a differing starting environment.
    """
    return utils.CLICommand(args, cwd, logger, env=env).run()


def execute_python_script(target_script_path: pathlib.Path or str, o3de_context: O3DEScriptExportContext = None) -> int:
    """
    Execute a new python script, using new or existing O3DEScriptExportContexts to streamline communication between multiple scripts
    :param target_script_path: The path to the python script to run.
    :param o3de_context: An O3DEScriptExportContext object that contains necessary data to run the target script. The target script can also write to this context to pass back to its caller.
    :return: return code upon success or failure
    """

    #Prepare import paths for export script ease of use
    #Allow for imports from calling script and the target script's local directory
    utils.add_to_system_path(pathlib.Path(__file__))
    utils.add_to_system_path(target_script_path)
    
    logger.info(f"Begin loading script '{target_script_path}'...")

    return utils.load_and_execute_script(target_script_path, o3de_context = o3de_context)


#Export Script entry point
def _run_export_script(args: argparse, passthru_args: list) -> int:
    logger.setLevel(args.log_level)

    export_script_path = args.export_script
    try:
        validate_export_script(export_script_path)
        project_path = utils.get_project_path_from_file(export_script_path, args.project_path)
        
        #prepare O3DE arguments for script
        o3de_context = O3DEScriptExportContext(export_script_path= export_script_path,
                                            project_path = project_path,
                                            engine_path = manifest.get_project_engine_path(project_path),
                                            logger= logger,
                                            args = passthru_args)

        return execute_python_script(export_script_path, o3de_context)

    except Exception as err:
        logger.error(str(err))
        return 1


#Argument handling
def add_parser_args(parser) -> None:
    parser.add_argument('-es', '--export-script', type=pathlib.Path, required=True, help="An external Python script to run")
    parser.add_argument('-pp', '--project-path', type=pathlib.Path, required=False,
                        help="Project to export. If not supplied, it will be inferred by the export script.")
    
    parser.add_argument('-v', '--verbosity', dest='log_level', default='ERROR',
                        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        help="Set the log verbosity level")
    
    parser.set_defaults(func=_run_export_script, accepts_partial_args=True)
    

def add_args(subparsers) -> None:
    export_subparser = subparsers.add_parser('export-project')
    add_parser_args(export_subparser)


def main() -> None:
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
