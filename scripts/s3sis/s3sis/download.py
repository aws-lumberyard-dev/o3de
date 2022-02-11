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


def add_args(subparsers):
    arg_parser = subparsers.add_parser('download')
    arg_parser.add_argument('-l', '--label', required=True, dest='label',
                            help='Label to be used to download')
    arg_parser.add_argument('-p', '--profile', required=False, dest='profile', default='default',
                            help='Profile to be used to upload, if not specified then default profile will be used')
    arg_parser.add_argument('-w', '--workspace', required=False, dest='workspace', default=os.getcwd(),
                            help='Workspace path to upload from, if not specified then current directory will be used')
    arg_parser.add_argument('--preserve-timestamp', required=False, dest='preserve_timestamp', action='store_true',
                            help='Preserve file timestamp, including last access timestamp, modified timestamp')
    arg_parser.add_argument('--preserve-attributes', required=False, dest='preserve_attributes', action='store_true',
                            help='Preserve file attributes. File permissions will be preserved when the file is uploaded or downloaded')
    arg_parser.add_argument('--preserve-empty-folders', required=False, dest='preserve_empty_folders', action='store_true',
                            help='Preserve empty folders. Empty folders will be uploaded or downloaded if this argument is provided')
    arg_parser.add_argument('--no-overwrite', required=False, dest='no_overwrite', action='store_true',
                            help='Don\'t overwrite changed local files. This argument will keep all local changes')
    arg_parser.add_argument('--cleanup', required=False, dest='cleanup', action='store_true',
                            help='Delete all the local files that are not tracked in manifest file from S3')


def run(args):
    # Load manifest file from local workspace.
    local_manifest = Manifest(args.workspace, args.profile, args.label)

    # Load manifest file from s3.
    s3client = S3Client(args.profile)
    s3_manifest = Manifest(args.workspace, args.profile, args.label, '.s3sis/s3_manifest.json')
    s3client.download_manifest(s3_manifest)
    s3_manifest.reload()
    s3_manifest.delete_file()

    # Get manifest difference
    manifest_diff = s3_manifest.difference_to(local_manifest)

    # Always download new files from S3.
    files_to_download = manifest_diff.new
    # If parameter --no-overwrite is not provided, download files whose content has changed.
    if not args.no_overwrite:
        files_to_download += manifest_diff.edited_md5
    s3client.download_multi_files(files_to_download)
    for file_info in files_to_download:
        # Overwrite file info of downloaded files in manifest file
        local_manifest.filelist[str(file_info.relpath)] = file_info.info()
        # Always create folders if parameter --preserve-empty-folders is provided.
        if args.preserve_empty_folders and not file_info.isfile:
            os.makedirs(file_info.abspath, exist_ok=True)

    # Modify timestamp if parameter --preserve-timestamp is provided.
    if args.preserve_timestamp:
        for file_info in manifest_diff.edited_timestamp:
            if file_info.isfile:
                # Overwrite file info of downloaded files in manifest file
                local_manifest.filelist[str(file_info.relpath)] = file_info.info()
                os.utime(file_info.abspath, (file_info.atimestamp, file_info.mtimestamp))

    # Modify file attributes if parameter --preserve-attributes is provided.
    if args.preserve_attributes:
        for file_info in manifest_diff.edited_attributes:
            if file_info.isfile:
                # Overwrite file info of downloaded files in manifest file
                local_manifest.filelist[str(file_info.relpath)] = file_info.info()
                os.chmod(file_info.abspath, file_info.attributes)

    # Delete local files that are not tracked in manifest file from s3 if parameter --cleanup is provided.
    if args.cleanup:
        for file_info in manifest_diff.deleted:
            # Overwrite file info of downloaded files in manifest file
            local_manifest.filelist.pop(str(file_info.relpath))
            # missing_ok parameter was added to Path.unlink() from Python 3.8
            if file_info.abspath.exists():
                file_info.abspath.unlink()

    # Update local manifest file on disk.
    local_manifest.write()
