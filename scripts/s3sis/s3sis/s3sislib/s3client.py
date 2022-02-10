#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from concurrent.futures import ThreadPoolExecutor, as_completed
import boto3
from botocore.exceptions import ClientError
from s3sis.s3sislib.config import Config
from s3sis.s3sislib.manifest import Manifest
from s3sis.s3sislib.fileinfo import FileInfo
from s3sis.s3sislib.util import error, warn
from pathlib import Path

class S3Client:
    def __init__(self, profile: str = 'default'):
        """
        s3 client of s3sis
        :param profile: s3sis profile to use
        """
        self.config = Config().get(profile)
        session = boto3.session.Session(region_name=self.config['region'], profile_name=self.config['aws-profile'])
        self.client = session.client("s3")
        self.project = self.config['project']
        self.bucket = self.config['bucket']
        self.thread_pool_size = int(self.config['thread-pool-size'])

    def object_key(self, file_info: FileInfo) -> str:
        """
        Get s3 object key where the file will be uploaded to.
        :param file_info: File info of the file to uploaded
        :return: Target s3 object key.
        """
        return f'{self.project}/objects/{file_info.md5}/object'

    def manifest_key(self, manifest: Manifest) -> str:
        """
        Get s3 key of manifest file.
        :param manifest: Manifest file
        :return: s3 key of manifest file.
        """
        return f'{self.project}/labels/{manifest.label}/manifest.json'

    def object_exists_on_s3(self, key: str) -> bool:
        """
        Check whether object exists on s3.
        :param key: Object key on s3
        :return: True if object exists on s3, otherwise, return False.
        """
        try:
            self.client.head_object(Bucket=self.bucket, Key=key)
        except ClientError:
            return False
        return True

    def upload_file(self, filename: str, object_key: str, overwrite: bool = False, extra_args: dict = None) -> None:
        """
        Upload a file to S3 bucket
        :param filename: File to upload
        :param bucket: Bucket to upload to
        :param object_key: S3 object name
        :param overwrite: Overwrite existing object on S3
        :param extra_args: Extra upload args
        :return: None
        """
        try:
            if overwrite or not self.object_exists_on_s3(object_key):
                self.client.upload_file(filename, self.bucket, object_key, ExtraArgs=extra_args)
        except ClientError as e:
            warn(f'Failed to upload file {filename}')
            raise e

    def upload_multi_files(self, files_to_upload: list, extra_args: dict = None) -> None:
        """
        Upload multiple files to S3.
        :param files_to_upload: A list of FileInfo object that stores the info of files to upload
        :param extra_args: Extra upload args
        :return: None
        """
        print(f'uploading {[file.relpath for file in files_to_upload]}')
        with ThreadPoolExecutor(max_workers=self.thread_pool_size) as executor:
            futures = {
                executor.submit(self.upload_file, str(file_info.abspath), self.object_key(file_info), extra_args):
                    file_info for file_info in files_to_upload if file_info.isfile
            }
            for future in as_completed(futures):
                exception = future.exception()
                if exception:
                    error(exception)

    def download_file(self, object_key: str, filename: str, extra_args: dict = None) -> bool:
        """
        Download a file to an S3 bucket.
        :param object_key: S3 object name to download
        :param filename: Target file name
        :param extra_args: Extra download args
        :return: True if file was downloaded, else False
        """
        try:
            filepath = Path(filename)
            filepath.parent.mkdir(exist_ok=True)
            self.client.download_file(self.bucket, object_key, filename, ExtraArgs=extra_args)
        except ClientError as e:
            warn(f'Failed to download file {filename}')
            raise e

    def download_multi_files(self, files_to_download: list, extra_args: dict = None) -> None:
        """
        Download multiple files from S3.
        :param files_to_download: A list of FileInfo object that stores the info of files to download
        :param extra_args: Extra upload args
        :return:
        """
        print(f'downloading {[file.relpath for file in files_to_download]}')
        with ThreadPoolExecutor(max_workers=self.thread_pool_size) as executor:
            futures = {
                executor.submit(self.download_file, self.object_key(file_info), str(file_info.abspath), extra_args):
                    file_info for file_info in files_to_download if file_info.isfile
            }
            for future in as_completed(futures):
                exception = future.exception()
                if exception:
                    error(exception)

    def upload_manifest(self, manifest: Manifest) -> None:
        """
        Upload manifest file to s3.
        :param manifest: Manifest object to upload
        :return: None
        """
        self.upload_file(str(manifest.manifest_path), self.manifest_key(manifest), overwrite=True)

    def download_manifest(self, manifest: Manifest) -> None:
        """
        Download manifest file from s3.
        :param manifest: Manifest file to download to
        :return: None
        """
        self.download_file(self.manifest_key(manifest), str(manifest.manifest_path))
