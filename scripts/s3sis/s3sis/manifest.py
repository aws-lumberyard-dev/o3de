#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
from s3sis.s3sislib.manifest import Manifest


def add_args(subparsers):
    arg_parser = subparsers.add_parser('manifest')
    arg_parser.add_argument('-p', '--profile', required=False, dest='profile', default='default',
                            help='Profile to be used to upload, if not specified then default profile will be used')
    arg_parser.add_argument('-w', '--workspace', required=False, dest='workspace', default=os.getcwd(),
                            help='Workspace path to generate manifest, if not specified then current directory will be used')
    arg_parser.add_argument('--local-validate', required=False, action='store_true', dest='local_validate',
                            help='Check if manifest file matches all local files.')
    arg_parser.add_argument('--regenerate', required=False, action='store_true', dest='regenerate',
                            help='Regenerate manifest file.')


def run(args):
    try:
        label = args.label
    except AttributeError:
        label = None
    manifest = Manifest(args.workspace, args.profile, label)
    if args.regenerate:
        manifest.regenerate()
    if args.local_validate:
        pass
