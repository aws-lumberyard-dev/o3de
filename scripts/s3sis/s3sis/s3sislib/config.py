#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from pathlib import Path
import configparser
from s3sis.s3sislib.util import error

DEFAULT_CONFIG_FILE = Path(__file__).parent.parent.parent / 's3sis.config'


class Config:
    def __init__(self, config_path: str = str(DEFAULT_CONFIG_FILE)):
        """
        Global config that stores s3sis settings.
        :param config_path: Global config file path
        """
        self._config_path = config_path
        self._config = configparser.ConfigParser()
        self._config.read(config_path)

    def get(self, profile: str = 'default') -> configparser.SectionProxy:
        """
        Get specified profile from global config file.
        :param profile: Profile name
        :return: Key value pair of settings of specified profile.
        """
        try:
            return self._config[profile]
        except KeyError:
            error(f'Profile {profile} doesn\'t exist in {self._config_path}')
