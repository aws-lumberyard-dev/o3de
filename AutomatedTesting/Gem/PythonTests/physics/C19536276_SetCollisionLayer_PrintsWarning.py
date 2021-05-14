"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: C19536276
Test Case Title: Verify that when an invalid or blank name is assigned to the collision layer
                 using Set Collision Layer node, a warning is printed on the console
"""


# fmt: off
class Tests():
    entity_enabled    = ("Successfully enabled test entity", "Failed to enable test entity")
    game_mode_entered = ("Successfully entered game mode",   "Failed to enter game mode")
    warning_found     = ("SectionTracer captured warnings",  "SectionTracer failed to capture warnings")
    game_mode_exited  = ("Successfully exited game mode",    "Failed to exit game mode")
# fmt: on


def C19536276_SetCollisionLayer_PrintsWarning():
    """
    Summary:
     Testing the warning output from setting Collision Layer with invalid data

    Level:
     An entity (named C19536276) exists in the level containing a PhysX Collider component with Collision Layer>Left and
     Collides With>GroupLeft. It also has a script canvas that attempt to set the Collision Layer (to invalid value).
     The test will enable the entity enter game mode to run the script canvas file.

    Expected Behavior:
     Inside the script canvas, since it is not setting a valid Collision Layer name, a warning should be printed to the
     console after entering game mode.

    Test Steps:
     1) Open test level
     2) Enable the test entity
     3) Start section tracer to look for warning
     4) Enter game mode to run script canvas
     5) Look for a warning
     6) Exit game mode

    Note:
     - This test file must be called from the Lumberyard Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor is required to observe the test results.

    :return: None
    """
    # Helper Files
    import ImportPathHelper as imports

    imports.init()
    from utils import Report
    from utils import TestHelper as helper
    from editor_entity_utils import EditorEntity as Entity
    from utils import Tracer

    # Lumberyard Imports
    import azlmbr

    # Constants
    EDITOR_IDLE_SECONDS = 5.0
    ACTIVE_STATUS = azlmbr.globals.property.EditorEntityStartStatus_StartActive
    EXPECTED_WARNING = "Could not find collision layer:matt. Does it exist in the physx configuration window?"

    helper.init_idle()
    # 1) Open test level
    helper.open_level("Physics", "C19536276_SetCollisionLayer_PrintsWarning")

    # 2) Enable the test entity
    test_entity = Entity.find_editor_entity("C19536276")
    test_entity.set_start_status("active")
    Report.result(Tests.entity_enabled, test_entity.get_start_status() == ACTIVE_STATUS)

    # 3) Start section tracer to look for warning
    with Tracer() as section_tracer:

        # 4) Enter game mode to run script canvas
        helper.enter_game_mode(Tests.game_mode_entered)

        # 5) Search for warning in logs
        warning_found = helper.wait_for_condition(
            lambda: EXPECTED_WARNING in [warning.message for warning in section_tracer.warnings], EDITOR_IDLE_SECONDS
        )

    Report.result(Tests.warning_found, warning_found)

    # 6) Exit game mode
    helper.exit_game_mode(Tests.game_mode_exited)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C19536276_SetCollisionLayer_PrintsWarning)
