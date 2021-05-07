"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: C1702834 // C1702823
Test Case Title: Opening pane // Closing pane
URLs of the test case: https://testrail.agscollab.com/index.php?/cases/view/1702834 and
    https://testrail.agscollab.com/index.php?/cases/view/1702823
"""


# fmt: off
class Tests():
    open_sc_window  = ("Script Canvas window is opened",    "Failed to open Script Canvas window")
    default_visible = ("All the panes visible by default",  "One or more panes do not visible by default")
    open_panes      = ("All the Panes opened successfully", "Failed to open one or more panes")
    close_pane      = ("All the Panes closed successfully", "Failed to close one or more panes")
# fmt: on


def C1702834_C1702823_Opening_Closing_Pane():
    """
    Summary:
     An empty level is loaded. The Script Canvas window is opened to verify if Script canvas panes can be opened and
     closed.

    Expected Behavior:
     The pane opens and closes successfully.

    Test Steps:
     1) Open test level
     2) Open Script Canvas window (Tools > Script Canvas)
     3) Restore default layout
     4) Verify if panes were opened by default
     5) Close the opened panes
     6) Open Script Canvas panes (Tools > <pane>)
     7) Restore default layout
     8) Close Script Canvas window

    Note:
     - This test file must be called from the Lumberyard Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os
    import sys

    sys.path.append(os.path.dirname(os.path.abspath(__file__)) + "/../automatedtesting_shared")

    from editor_test_helper import EditorTestHelper
    from utils import TestHelper as helper
    from utils import Report
    from PySide2 import QtWidgets
    import Tests.ly_shared.pyside_utils as pyside_utils
    import azlmbr.legacy.general as general

    PANE_WIDGETS = ("NodePalette", "VariableManager")

    def click_menu_option(window, option_text):
        action = pyside_utils.find_child_by_pattern(window, {"text": option_text, "type": QtWidgets.QAction})
        action.trigger()

    def find_pane(window, pane_name):
        return window.findChild(QtWidgets.QDockWidget, pane_name)

    def is_pane_visible(window, pane_name):
        pane = find_pane(window, pane_name)
        return pane.isVisible()

    # 1) Open test level
    EditorTestHelper("").open_level(os.path.join("scripting", "EmptyLevel"), True)

    # 2) Open Script Canvas window (Tools > Script Canvas)
    general.open_pane("Script Canvas")
    is_sc_visible = helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)
    Report.result(Tests.open_sc_window, is_sc_visible)

    # 3) Restore default layout
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    click_menu_option(sc, "Restore Default Layout")

    # 4) Verify if panes were opened by default
    PANES_VISIBLE = all(is_pane_visible(sc, pane) for pane in PANE_WIDGETS)
    Report.critical_result(Tests.default_visible, PANES_VISIBLE)

    # 5) Close the opened panes
    for item in PANE_WIDGETS:
        pane = sc.findChild(QtWidgets.QDockWidget, item)
        pane.close()
        if pane.isVisible():
            Report.info(f"Failed to close pane : {item}")

    PANES_VISIBLE = any(is_pane_visible(sc, pane) for pane in PANE_WIDGETS)
    Report.result(Tests.close_pane, not PANES_VISIBLE)

    # 6) Open Script Canvas panes (Tools > <pane>)
    click_menu_option(sc, "Node Palette")
    click_menu_option(sc, "Variable Manager")
    PANES_VISIBLE = helper.wait_for_condition(lambda: all(is_pane_visible(sc, pane) for pane in PANE_WIDGETS), 2.0)
    Report.result(Tests.open_panes, PANES_VISIBLE)

    # 7) Restore default layout
    # Needed this step to restore to default in case of test failure
    click_menu_option(sc, "Restore Default Layout")

    # 8) Close Script Canvas window
    sc.close()


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C1702834_C1702823_Opening_Closing_Pane)