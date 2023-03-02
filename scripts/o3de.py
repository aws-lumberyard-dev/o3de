#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import pathlib
import sys

logger = logging.getLogger('o3de')

scripts_using_partial_args = set()

def add_args(parser: argparse.ArgumentParser) -> None:
    def mark_using_partial_args(module):
        scripts_using_partial_args.add(module.__name__.split('.')[-1])

    """
    add_args is called to add expected parser arguments and subparsers arguments to each command such that it can be
    invoked by o3de.py
    Ex o3de.py can invoke the register  downloadable commands by importing register,
    call add_args and execute: python o3de.py register --gem-path "C:/TestGem"
    :param parser: the caller instantiates an ArgumentParser and passes it in here
    """

    subparsers = parser.add_subparsers(help='To get help on a sub-command:\no3de.py <sub-command> -h',
                                       title='Sub-Commands')

    # As o3de.py shares the same name as the o3de package attempting to use a regular
    # from o3de import <module> line tries to import from the current o3de.py script and not the package
    # So the {current script directory} / 'o3de' is added to the front of the sys.path
    script_dir = pathlib.Path(__file__).parent
    o3de_package_dir = (script_dir / 'o3de').resolve()
    # add the scripts/o3de directory to the front of the sys.path
    sys.path.insert(0, str(o3de_package_dir))
    from o3de import engine_properties, engine_template, gem_properties, \
        global_project, register, print_registration, get_registration, \
        enable_gem, disable_gem, project_properties, sha256, download, export_project
    # Remove the temporarily added path
    sys.path = sys.path[1:]

    # global project
    global_project.add_args(subparsers)

    # engine template
    engine_template.add_args(subparsers)

    # registration
    register.add_args(subparsers)

    # show registration
    print_registration.add_args(subparsers)

    # get registration
    get_registration.add_args(subparsers)

    # add a gem to a project
    enable_gem.add_args(subparsers)

    # remove a gem from a project
    disable_gem.add_args(subparsers)

    # modify engine properties
    engine_properties.add_args(subparsers)

    # modify project properties
    project_properties.add_args(subparsers)

    # modify gem properties
    gem_properties.add_args(subparsers)

    # sha256
    sha256.add_args(subparsers)

    # download
    download.add_args(subparsers)

    #export_project
    export_project.add_args(subparsers)
    mark_using_partial_args(export_project)


if __name__ == "__main__":
    # parse the command line args
    the_parser = argparse.ArgumentParser()

    # add args to the parser
    add_args(the_parser)

    # if empty print help
    if len(sys.argv) == 1:
        the_parser.print_help(sys.stderr)
        sys.exit(1)

    command_uses_partial_args = sys.argv[1] in scripts_using_partial_args
    # parse args
    if command_uses_partial_args:
        #argparse stores unknown arguments separately as a tuple, not packed in the same NameSpace as known arguments
        known_args, unknown_args = the_parser.parse_known_args()
        ret = known_args.func(known_args, unknown_args) if hasattr(known_args, 'func') else 1
    else:
        the_args = the_parser.parse_args()
        ret = the_args.func(the_args) if hasattr(the_args, 'func') else 1

    # run
    logger.info('Success!' if ret == 0 else 'Completed with issues: result {}'.format(ret))

    # return
    sys.exit(ret)
