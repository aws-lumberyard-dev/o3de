#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

def add_args(subparsers):
    arg_parser = subparsers.add_parser('configure')
    arg_parser.add_argument('-p', '--profile', required=False, dest='profile', default='default',
                            help='Profile to be configured, if not specified then default profile will be used')


def run(ars):
    pass
