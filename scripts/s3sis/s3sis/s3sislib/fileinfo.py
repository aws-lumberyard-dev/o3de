#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import hashlib
from pathlib import Path


class FileInfo:
    def __init__(self, relpath: str, workspace: str = os.getcwd(), extra_info: dict = None):
        """
        File info calculated from local file or read from manifest file.
        :param relpath: File relative path to workspace
        :param workspace: Workspace path where the file is located
        :param extra_info: If provided then read file info from manifest file, otherwise, calculate file info from local file
        """
        self.relpath = Path(relpath)
        self.workspace = Path(workspace)
        self.abspath = self.workspace / self.relpath
        # extra_info is read from manifest file
        if extra_info:
            self.isfile = extra_info['isfile']
            if self.isfile:
                self.md5 = extra_info['md5']
                self.size = extra_info['size']
                self.atimestamp = extra_info['atimestamp']
                self.mtimestamp = extra_info['mtimestamp']
                self.attribute = extra_info['attribute']
        # Calculate local file info if extra_info is not provided.
        else:
            self.isfile = self.abspath.is_file()
            if self.isfile:
                stat = self.abspath.stat()
                self.size = stat.st_size
                self.atimestamp = stat.st_atime
                self.mtimestamp = stat.st_mtime
                self.attribute = stat.st_mode

    def info(self) -> dict:
        """
        File info in json format.
        :return: File info in json format
        """
        if self.isfile:
            return {
                'isfile': self.isfile,
                'md5': self.checksum_md5(),
                'size': self.size,
                'atimestamp': self.atimestamp,
                'mtimestamp': self.mtimestamp,
                'attribute': self.attribute,
            }
        else:
            return {
                'isfile': self.isfile
            }

    def checksum_md5(self, block_size=512 * 16) -> str:
        """
        Calculate md5 hash of the file.
        :param filename: File name
        :param block_size: Block size used to read from file, default to 512 * 16 as md5 digest size is 128 bits (16 bytes)
        :return: md5 hash of the file
        """
        if not self.isfile:
            return ''
        if hasattr(self, 'md5'):
            return self.md5

        m = hashlib.md5()
        with self.abspath.open('rb') as f:
            for chunk in iter(lambda: f.read(block_size), b''):
                m.update(chunk)
        self.md5 = m.hexdigest()
        return self.md5
