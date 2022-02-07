#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
from s3sis.s3sislib.config import Config
from s3sis.s3sislib.fileinfo import FileInfo
from s3sis.s3sislib.util import error
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed

DEFAULT_MANIFEST_RELPATH = '.s3sis/manifest.json'


class ManifestDiff:
    def __init__(self):
        self.new = []
        self.edited_md5 = []
        self.edited_timestamp = []
        self.edited_attributes = []
        self.deleted = []


class Manifest:
    def __init__(self, workspace: str, profile: str = 'default', label: str = None, manifest_relpath: str = DEFAULT_MANIFEST_RELPATH):
        """
        Manifest file that stores file info of the files to be transferred.
        :param workspace: Workspace path
        :param profile: Profile to use from global config file
        :param label: Label to use in manifest file
        :param manifest_relpath: Manifest file's relative path to workspace path
        """
        self.config = Config().get(profile)
        self.thread_pool_size = int(self.config['thread-pool-size'])
        self.workspace = Path(workspace)
        self.manifest_path = Path(workspace) / manifest_relpath
        self.filelist = {}
        self.label = None
        self.reload()
        if label:
            self.label = label

    def reload(self) -> None:
        """
        Reload manifest file from disk if manifest file exists.
        :return: None
        """
        if self.manifest_path.exists():
            with self.manifest_path.open() as source:
                data = json.load(source)
                self.label = data['label']
                self.filelist = data['filelist']

    def delete_file(self) -> None:
        """
        Delete manifest file from disk if exists.
        :return:
        """
        if self.manifest_path.exists():
            self.manifest_path.unlink()

    def regenerate(self) -> None:
        """
        Generate manifest file.
        :return: None
        """
        self.filelist = {}
        files = self.workspace.glob('**/*')
        with ThreadPoolExecutor(max_workers=self.thread_pool_size) as executor:
            futures = {
                executor.submit(self.single_local_file_info, filename): filename
                # Exclude manifest file info in manifest file
                for filename in files if filename != self.manifest_path
            }
            for future in as_completed(futures):
                exception = future.exception()
                if exception:
                    error(exception)
                filename = futures[future]
                relpath = str(filename.relative_to(self.workspace))
                self.filelist[relpath] = future.result()
        self.write()

    def write(self) -> None:
        """
        Write manifest data to disk.
        :return: None
        """
        data = {
            'project': self.config['project'],
            'label': self.label,
            'filelist': self.filelist
        }
        self.manifest_path.parent.mkdir(parents=True, exist_ok=True)
        with self.manifest_path.open('w') as output:
            json.dump(data, output, indent=4)

    def single_local_file_info(self, filename: Path) -> dict:
        """
        Get file info of a single local file.
        :param filename: File name of the local file
        :return: local file info in json format.
        """
        relpath = str(filename.relative_to(self.workspace))
        file_info = FileInfo(relpath, str(self.workspace))
        return file_info.info()

    def file_info_list(self) -> list[FileInfo]:
        """
        Generate a list of FileInfo object stored in manifest file.
        :return: A list of FileInfo object stored in manifest file.
        """
        file_info_list = []
        for filepath, extra_info in self.filelist.items():
            file_info = FileInfo(filepath, str(self.workspace), extra_info)
            file_info_list.append(file_info)
        return file_info_list

    def difference_to(self, other_manifest: "Manifest", ignore_timestamp: bool = True, ignore_attribute: bool = True) -> ManifestDiff:
        """
        Compare current manifest file with other manifest file and return the difference.
        :param other_manifest: Other manifest file to compare to
        :param ignore_timestamp: Ignore timestamp when comparing two manifest
        :param ignore_attribute: Ignore attribute when comparing two manifest
        :return: A dict stores the difference between two manifest.
                 Key is the state of the FileInfo difference (added, edited, deleted).
        """
        manifest_diff = ManifestDiff()
        filelist = self.filelist
        other_filelist = other_manifest.filelist
        edited_candidates = {}
        # Find added files
        for filename, info in filelist.items():
            if filename not in other_filelist:
                file_info = FileInfo(filename, str(self.workspace), info)
                manifest_diff.new.append(file_info)
            else:
                edited_candidates[filename] = info
        # Find deleted files
        for filename, info in other_filelist.items():
            if filename not in filelist:
                file_info = FileInfo(filename, str(self.workspace), info)
                manifest_diff.deleted.append(file_info)
        # Find edited files
        for filename, info in edited_candidates.items():
            if not filelist[filename]['isfile']:
                continue
            if filelist[filename]['md5'] != other_filelist[filename]['md5']:
                file_info = FileInfo(filename, str(self.workspace), info)
                manifest_diff.edited_md5.append(file_info)
                continue
            if not ignore_timestamp:
                timestamps = ['atimestamp', 'mtimestamp']
                for timestamp in timestamps:
                    if filelist[filename][timestamp] != other_filelist[filename][timestamp]:
                        file_info = FileInfo(filename, str(self.workspace), info)
                        manifest_diff.edited_timestamp.append(file_info)
                        continue
            if not ignore_attribute:
                if filelist[filename]['attribute'] != other_filelist[filename]['attribute']:
                    file_info = FileInfo(filename, str(self.workspace), info)
                    manifest_diff.edited_attributes.append(file_info)
        return manifest_diff

