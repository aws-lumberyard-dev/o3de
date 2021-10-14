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

"""! @brief This is the entry point for the DCCsi Launcher. Come up with a better description """

##
# @file main.py
#
# @brief The Launcher is a convenience tool for accessing and launching all DCC tools and productivity scripts
# contained within the DCCsi system.
#
# @section Launcher Description
# The Launcher (and main.py module) is an entrypoint for launching DCC-related productivity scripts within the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section Launcher Notes
# - Comments are Doxygen compatible
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Slot
from data.configuration import Configuration
from data.model import LauncherModel
from data import project_constants as constants
from Python.baselogger import logging
from navigation import Navigation
from sections import splash, tools, projects, output, setup, help
from box import Box
import sys
import os

_LOGGER = logging.getLogger('Launcher.main')

# TODO - Tool directories should all be Camel case, and the Launcher can split at caps


class Launcher(QtWidgets.QMainWindow):
    """! The Launcher base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self):
        super(Launcher, self).__init__()

        self.app = QtWidgets.QApplication.instance()
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 1280, 720)
        self.setObjectName('DCCsiLauncher')
        self.setWindowTitle('DCCsi Launcher')
        self.setWindowIcon(QtGui.QIcon(str(constants.ICON_PATH)))
        self.isTopLevel()

        self.model = LauncherModel()
        self.configuration = Configuration()
        self.content = ContentContainer(self.model)
        self.setCentralWidget(self.content)

        for target_dict in self.model.tables.values():
            self.configuration.set_variables({k: v for k, v in target_dict.items()})

        self.statusBar = QtWidgets.QStatusBar()
        self.statusBar.setStyleSheet('background-color: rgb(35, 35, 35)')
        self.setStatusBar(self.statusBar)
        self.load_bar = QtWidgets.QProgressBar()
        self.load_bar.setFixedSize(200, 20)
        self.statusBar.addPermanentWidget(self.load_bar)
        self.statusBar.showMessage('Ready.')


class ContentContainer(QtWidgets.QWidget):
    """! This sets up the content window and navigation system

    All changes to window content move through this class, which is driven by user events.
    """
    def __init__(self, model):
        super(ContentContainer, self).__init__()

        self.app = QtWidgets.QApplication.instance()
        self.model = model
        self.sections = Box({})
        self.current_section = None
        self.main_layout = QtWidgets.QVBoxLayout(self)
        self.main_layout.setAlignment(QtCore.Qt.AlignTop)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(2)
        self.navigation = Navigation(self.model)
        self.navigation.section_requested.connect(self.change_section)
        self.main_layout.addWidget(self.navigation)
        self.sections_layout = QtWidgets.QStackedLayout()
        self.main_layout.addLayout(self.sections_layout)
        self.build()

    def build(self):
        """! This function sets up the main content widget that contains all of the section content. Each
        section is a separate class (separated and stored inside the Launcher package's 'sections' directory),
        loaded into a stacked layout that serves up content based on main navigation clicks
        """
        self.sections['splash'] = [splash.Splash(self.model), 0]
        self.sections['tools'] = [tools.Tools(self.model), 1]
        self.sections['projects'] = [projects.Projects(self.model), 2]
        self.sections['output'] = [output.Output(self.model), 3]
        self.sections['setup'] = [setup.Setup(self.model), 4]
        self.sections['help'] = [help.Help(self.model), 5]
        for section in self.sections.values():
            self.sections_layout.addWidget(section[0])

        self.change_section('splash')

    @Slot(str)
    def change_section(self, target_section: str):
        """! This is the central point for changing the content container based on a user clicking on a main
        navigation button.

        @param  target_section This is the string passed in the signal that corresponds to requested section
        """
        _LOGGER.info(f'Section change event:::  Switch to {target_section}')
        if self.current_section:
            self.sections[self.current_section][0].close_section()
        self.current_section = target_section
        self.sections_layout.setCurrentIndex(self.sections[self.current_section ][1])
        self.sections[self.current_section][0].open_section()


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    config = Configuration()
    with open(os.environ['GLOBAL_QT_STYLESHEET'], 'r') as stylesheet:
        app.setStyleSheet(stylesheet.read())

    launcher = Launcher()
    launcher.show()
    sys.exit(app.exec_())

