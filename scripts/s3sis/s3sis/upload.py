#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
from s3sis.s3sislib.manifest import Manifest
from s3sis.s3sislib.s3client import S3Client
from botocore.exceptions import ClientError


def add_args(subparsers):
    arg_parser = subparsers.add_parser('upload')
    arg_parser.add_argument('-l', '--label', required=True, dest='label',
                            help='Label to be used to upload or download')
    arg_parser.add_argument('-w', '--workspace', required=False, dest='workspace', default=os.getcwd(),
                            help='Workspace path to upload from, if not specified then current directory will be used')
    arg_parser.add_argument('-p', '--profile', required=False, dest='profile', default='default',
                            help='Profile to be used to upload, if not specified then default profile will be used')


def run(args):
    # Always regenerate manifest file in local workspace.
    local_manifest = Manifest(args.workspace, args.profile, args.label)
    local_manifest.regenerate()

    # Load manifest file from s3 if exists.
    s3client = S3Client(args.profile)
    s3_manifest = Manifest(args.workspace, args.profile, args.label, '.s3sis/s3_manifest.json')
    try:
        s3client.download_manifest(s3_manifest)
        s3_manifest.reload()
        s3_manifest.delete_file()
    except ClientError:
        pass

    # Get manifest difference.
    manifest_diff = local_manifest.difference_to(s3_manifest)

    # Only upload new files and files whose content has changed.
    files_to_upload = manifest_diff.new + manifest_diff.edited_md5

    s3client.upload_multi_files(files_to_upload)
    s3client.upload_manifest(local_manifest)
