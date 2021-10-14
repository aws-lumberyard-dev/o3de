# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

"""! @brief This module contains several common utilities/operations for Python when creating tools for the DCCsi. """

##
# @file general_utilities.py
#
# @brief This module contains several helper functions useful when creating Python tool scripts.
#
# @section General Utilities Description
# Python module containing many commonly needed helper functions for Python tool development in the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section QT Utilities Notes
# - Comments are Doxygen compatible

from SDK.Python import general_utilities as helpers
from pathlib import Path
from box import Box
import logging
import os
import re

_LOGGER = logging.getLogger('Launcher.model')


class LauncherModel:
    def __init__(self):
        self.database_path = None
        self.db = None
        self.cursor = None
        self.tools = Box({})
        self.projects = Box({})
        self.applications = Box({})
        self.tables = {
            'tools': self.tools,
            'projects': self.projects,
            'applications': self.applications
        }
        self.tools_headers = ['appName', 'toolName', 'toolLocation', 'toolMarkdown', 'toolDocumentation', 'toolVersion']
        self.projects_headers = ['projectName', 'projectPath']
        self.applications_headers = ['appName', 'appVersion', 'appPath']
        self.launcher_sections = ['Tools', 'Projects', 'Output', 'Setup', 'Help']
        self.connect_to_launcher_db(os.environ['DB_PATH'])

    def connect_to_launcher_db(self, database_path: Path):
        self.database_path = database_path
        self.db = helpers.get_database(self.database_path)
        if self.db:
            self.cursor = self.db.cursor()
            self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
            tables = self.cursor.fetchall()
            self.extract_db_values() if tables else self.initialize_db_values()

    def initialize_db_values(self):
        _LOGGER.info('Initializing DB Values...')
        self.set_applications()
        self.set_projects()
        self.set_tools()
        self.close_database()

    def extract_db_values(self):
        _LOGGER.info('Extracting DB Values...')
        database_values = helpers.get_database_values(self.cursor, [*self.tables])
        for key, values in database_values.items():
            for value in values:
                self.tables[key].update({value[0]: value[1]})
        self.close_database()

    def set_applications(self):
        helpers.create_table(self.db, self.cursor, 'applications', [
            'app varchar(20) NOT NULL',
            'path varchar(20) NOT NULL'
        ])
        self.applications = helpers.get_json_data(Path(os.environ['DCC_PATHS_JSON']))
        locked = self.applications['locked']
        for index, (key, value) in enumerate(self.applications['locations'].items()):
            if not locked:
                value = helpers.validate_path(Path(value.replace('<username>', os.getenv('username'))))
            self.applications[key] = value
            helpers.create_table_entry(self.db, self.cursor, 'applications', [
                self.format_for_query(key),
                self.format_for_query(value.__str__())
            ])

    def set_projects(self):
        helpers.create_table(self.db, self.cursor, 'projects', [
            'project varchar(20) NOT NULL',
            'path varchar(20) NOT NULL'
        ])
        flagged_directories = ['Cache', 'DummyProject', 'Template']
        for index, file_path in enumerate(helpers.get_files_by_name(Path(os.environ['LY_DEV']), 'project.json')):
            if not [x for x in flagged_directories if x in file_path.parts]:
                project_location = file_path.parent
                project_data = Box(helpers.get_json_data(file_path))
                project_name = project_data.project_name
                self.projects[project_name] = project_location
                helpers.create_table_entry(self.db, self.cursor, 'projects', [
                    self.format_for_query(project_name),
                    self.format_for_query(project_location)
                ])

    def set_tools(self):
        helpers.create_table(self.db, self.cursor, 'tools', [
            'tool varchar(20) NOT NULL',
            'path varchar(20) NOT NULL',
        ])
        for index, file_path in enumerate(helpers.get_files_by_name(Path(os.environ['DCCSIG_PATH']), 'README.md')):
            if helpers.validate_markdown(file_path):
                markdown_info = helpers.get_markdown_information(file_path)
                tool = re.sub(r'((?<=[a-z])[A-Z]|(?<!\A)[A-Z](?=[a-z]))', r' \1', file_path.parent.name)
                self.tools.update({
                    tool: {
                        'base_directory': file_path.parent,
                        'markdown_file': file_path.stem,
                        'entry_file': markdown_info['Entry File']
                    }})
                helpers.create_table_entry(self.db, self.cursor, 'tools', [
                    self.format_for_query(tool),
                    self.format_for_query(file_path.parent.__str__()),
                ])

    def get_application_path(self, application: str) -> Path:
        if application in self.applications.keys():
            return self.applications[application]
        return Path('')
    
    def get_table_names(self) -> list:
        return [*self.tables]

    def get_sections(self) -> list:
        return self.launcher_sections

    def close_database(self):
        self.db.close()
        self.db = None
        self.cursor = None

    @staticmethod
    def format_for_query(value):
        return f"\'{value}\'"

