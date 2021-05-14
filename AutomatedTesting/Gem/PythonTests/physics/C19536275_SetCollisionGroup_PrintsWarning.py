"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: C19536275
Test Case Title: Verify that when an invalid or blank name is assigned to the collision group using Set Collision Group
    node, a warning is thrown on console
"""


# fmt: off
class Tests():
    # C19536275_0
    C19536275_0_enter_game_mode = ("Successfully entered game mode first run",       "Failed to enter game mode first run")
    C19536275_0_exit_game_mode  = ("Successfully exited game mode first run",        "Failed to exit game mode first run")
    C19536275_0_enabled         = ("Successfully enabled C19536275_0",               "Failed to enable C19536275_0")
    C19536275_0_disabled        = ("Successfully disabled C19536275_0",              "Failed to disable C19536275_0")
    C19536275_0_warning_found   = ("Successfully found expected warning first run",  "Failed to find expected warning first run")
    # C19536275_1 Tests
    C19536275_1_enter_game_mode = ("Successfully entered game mode second run",      "Failed to enter game mode second run")
    C19536275_1_exit_game_mode  = ("Successfully exited game mode second run",       "Failed to exit game mode second run")
    C19536275_1_enabled         = ("Successfully enabled C19536275_1",               "Failed to enable C19536275_1")
    C19536275_1_warning_found   = ("Successfully found expected warning second run", "Failed to find expected warning second run")
# fmt: on


def C19536275_SetCollisionGroup_PrintsWarning():
    """
    Summary:
     The test will enable the test entities one at a time and enter game mode to run the script canvas files attached
     respectively.

    Level:
     C19536275_0 : Entity with PhysX Collider component which is Inactive on start.
      It has C19536275_SetCollisionGroupEmpty.scriptcanvas that attempts to set the Collision Group to empty.
     C19536275_1 : Entity with PhysX Collider component which is Inactive on start.
      It has C19536275_SetCollisionGroupInvalid.scriptcanvas that attempts to set the Collision Group to Invalid value.

    Expected Behavior:
     For each script, since it is not a valid Collision Group name, a warning should be printed to the console after
     entering game mode

    Test Steps:
     1) Open empty level
     2) Run enable_and_run_entity (repeating steps) for first entity
        2a) Enable the entity
        2b) Start section tracer to look for warning
        2c) Enter game mode to run script canvas
        2d) Check for a warning
        2e) Exit game mode
     3) Disable the first entity
     4) Run Repeatable Steps for second entity (2a-2e)

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

    # Lumberyard Import
    import azlmbr

    EDITOR_WAIT_SECONDS = 5.0
    EXPECTED_WARNING_0 = "Could not find collision group:. Does it exist in the physx configuration window?"
    EXPECTED_WARNING_1 = "Could not find collision group:matt. Does it exist in the physx configuration window?"
    ACTIVE_STATUS = azlmbr.globals.property.EditorEntityStartStatus_StartActive
    INACTIVE_STATUS = azlmbr.globals.property.EditorEntityStartStatus_StartInactive

    def enable_and_run_entity(entity_name, expected_warning):
        # a) Enable the entity
        test_entity = Entity.find_editor_entity(entity_name)
        test_entity.set_start_status("active")
        Report.result(Tests.__dict__[f"{entity_name}_enabled"], test_entity.get_start_status() == ACTIVE_STATUS)

        # b) Start section tracer to look for warning
        with Tracer() as section_tracer:

            # c) Enter game mode to run script canvas
            helper.enter_game_mode(Tests.__dict__[f"{entity_name}_enter_game_mode"])

            # d) Check for a warning
            expected_warning_found = helper.wait_for_condition(
                lambda: expected_warning in [warning.message for warning in section_tracer.warnings],
                EDITOR_WAIT_SECONDS,
            )
            Report.result(Tests.__dict__[f"{entity_name}_warning_found"], expected_warning_found)

        # e) Exit game mode
        helper.exit_game_mode(Tests.__dict__[f"{entity_name}_exit_game_mode"])

        return test_entity

    helper.init_idle()
    # 1) Open empty level
    helper.open_level("Physics", "C19536275_SetCollisionGroup_PrintsWarning")

    # 2) Run enable_and_run_entity (repeating steps) for first entity
    entity_0 = enable_and_run_entity("C19536275_0", EXPECTED_WARNING_0)

    # 3) Disable the first entity
    entity_0.set_start_status("inactive")
    Report.result(Tests.C19536275_0_disabled, entity_0.get_start_status() == INACTIVE_STATUS)

    # 4) Run Repeatable Steps for second entity
    enable_and_run_entity("C19536275_1", EXPECTED_WARNING_1)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C19536275_SetCollisionGroup_PrintsWarning)
