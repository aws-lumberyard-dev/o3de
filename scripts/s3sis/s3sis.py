#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import sys
import importlib
import argparse
from s3sis.s3sislib.util import error

SUPPORTED_SUB_COMMANDS = ['upload', 'download', 'configure', 'manifest']
SUB_COMMAND_MODULES = {sub_command: importlib.import_module(f's3sis.{sub_command}')
                       for sub_command in SUPPORTED_SUB_COMMANDS}


def parse_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='sub-command help')
    for sub_command in SUPPORTED_SUB_COMMANDS:
        SUB_COMMAND_MODULES[sub_command].add_args(subparsers)
    return parser.parse_args()


def main():
    try:
        sub_command = sys.argv[1]
    except IndexError:
        error(f'You must provide a sub-command: {" | ".join(SUPPORTED_SUB_COMMANDS)}')
    if sub_command not in SUPPORTED_SUB_COMMANDS:
        error(f'Unknown sub-command {sub_command}. You must provide a valid sub-command: {" | ".join(SUPPORTED_SUB_COMMANDS)}')
    args = parse_args()
    SUB_COMMAND_MODULES[sub_command].run(args)


if __name__ == '__main__':
    main()
