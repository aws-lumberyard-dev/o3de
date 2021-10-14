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

"""! @brief This module contains several common utilities/operations for use with QT. """

##
# @file qt_utilities.py
#
# @brief This module contains several common utilities/operations for use with QT.
#
# @section QT Utilities Description
# Python module containing most of the commonly needed helper functions for QT Widgets in the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section QT Utilities Notes
# - Comments are Doxygen compatible


# from __future__ import annotations
# from typing import Callable
from pathlib import Path
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import QProcess


def populate_table(table_data: list, text_alignment: list, editable: bool, cell_color: dict,
                   min_size: list, block_signals: bool):
    pass


def set_cell_values(text: str, background_color: QtGui.QColor, set_flags: list, bold: bool):
    pass


def set_combobox_values(values: list, selected: int):
    pass


def set_item_visibility(ui_items: list, is_visible: bool):
    pass


def create_material(material_type: str, material_values: dict):
    pass


def get_application_path(dcc_name: str):
    pass


def set_qprocess():
    pass


def get_qprocess_state(state: QProcess):
    pass


def handle_stderr():
    pass


def handle_stdout(callback: Callable):
    pass


def create_spacer_line():
    """! Convenience function for adding separation line to the UI. """
    layout = QtWidgets.QHBoxLayout()
    line = QtWidgets.QLabel()
    line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
    line.setLineWidth(1)
    line.setFixedHeight(10)
    layout.addWidget(line)
    layout.setContentsMargins(0, 0, 0, 0)
    return layout


def create_info_dialog(window_title: str, message: str, buttons: list) -> list:
    pass


def create_file_chooser(start_directory: Path, file_type: str, single_selection: bool) -> list:
    pass

"""
Function annotations Example:

    from __future__ import annotations
    from typing import Callable

    def func_a(p:int) -> int:
        return p*5

    def func_b(func: Callable[[int], int]) -> int:
        return func(3)

    if __name__ == "__main__":
        print(func_b(func_a))


Reusable QT Components

Location:
E:\Depot\o3de_dccsi\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\QT\qt_utilities.py


- Load bar
- Info/status bar
- Combobox
- Table



Set up threading




"""


