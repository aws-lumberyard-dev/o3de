"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID : C22724380
Test Case Title : Check that world ray cast scene queries work with collision groups set in script canvas nodes
URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/22724380
"""


# fmt: off
class Tests():
    enter_game_mode              = ("Entered game mode",                               "Failed to enter game mode")
    exit_game_mode               = ("Exited game mode",                                "Failed to exit game mode")
    all_raycasts_deactivated     = ("All the raycasts deactivated successfully",       "One or more raycasts failed to deactivate")
    expected_lines_found         = ("All expected lines found",                        "Failed to find one or more expected line")
    # entities found
    find_collider_entities       = ("All collider entities found",                     "One or more collider entities not found")
    find_ray_cast_world_entities = ("Raycasts world entities found",                   "One or more Raycast world entities not found")
    find_script_entity           = ("Script entity found",                             "Script entity not found")
    # entity results
    ray_cast_world_unknown       = ("ray_cast_world_unknown successfully deactivated", "ray_cast_world_unknown failed to deactivate")
    ray_cast_world_all           = ("ray_cast_world_all successfully deactivated",     "ray_cast_world_all failed to deactivate")
    ray_cast_world_none          = ("ray_cast_world_none successfully deactivated",    "ray_cast_world_none failed to deactivate")
    ray_cast_world_empty         = ("ray_cast_world_empty successfully deactivated",   "ray_cast_world_empty failed to deactivate")
# fmt: on


class LogLines:
    """
    All the World Raycasts are expected to hit the top row of entities except the one with 'none' collision group.
    When expected lines found in logs, values from below expected_lines will be marked as True.
    """

    expected_lines = [
        "RayCastWorld Unknown was hit : true",
        "RayCastWorld All was hit: true",
        "RayCastWorld None was hit: false",
        "RayCastWorld Empty was hit: true",
    ]


def C22724380_ScriptCanvas_RayCast_CollisionGroup():
    """
    Summary:
     Verify that world ray cast scene queries work with collision groups set in script canvas nodes

    Level Description:
     2 box collider entities, one on top of another. Named box1 and box2.
     2 sphere colliders, one on top of another. Named sphere1 and sphere2.
     2 capsule colliders, one on top of another. Named capsule1 and capsule2.
     4 raycast entities that start activated but will get deactivated if expected script canvas output is attained.
        Named ray_cast_world_unknown, ray_cast_world_all, ray_cast_world_none and ray_cast_world_empty.
     1 entity named 'script' containing script canvas component to run C22724380_scriptcanvas_raycasts.scriptcanvas.

    Expected Behavior:
     When entered into Gamemode , all the World Raycasts will hit the top row of entities except the one with 'none'
     collision group. Raycast entities get deactivated when expected output is attained.

    Test Steps:
     1) Open level
     2) Retrieve and validate entities
     3) Make sure that all the entities are in Active state
     4) Start tracer to find expected lines
     5) Enter game mode
     6) Verify if all Raycasts worlds deactivated
     7) Find the expected lines
     8) Disconnect handlers so they don't get called implicitly on level cleanup
     9) Exit game mode

    Note:
    - This test file must be called from the Lumberyard Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Helper imports
    import ImportPathHelper as imports

    imports.init()
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer
    from editor_entity_utils import EditorEntity

    # Lumberyard Imports
    import azlmbr
    import azlmbr.legacy.general as general

    # Constants
    READLINE_TIMEOUT = 2 # seconds
    RAYCAST_DEACTIVATE_TIMEOUT = 2 # seconds
    COLLIDERS = ["box1", "box2", "sphere1", "sphere2", "capsule1", "capsule2"]
    RAYCASTS = ["ray_cast_world_unknown", "ray_cast_world_all", "ray_cast_world_none", "ray_cast_world_empty"]

    class Raycast:
        def __init__(self, name):
            self.id = general.find_game_entity(name)
            self.name = name
            self.activated = True  # All entities are activated on start
            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityDeactivated", self.on_deactivation)

        # Callback: gets called when entity successfully deactivated. This is expected for Raycast entities.
        def on_deactivation(self, args):
            if args[0].Equal(self.id):
                Report.success(Tests.__dict__[self.name])
                self.activated = False

        # Disconnects event handler.
        def disconnect(self):
            self.handler.disconnect()
            self.handler = None

    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "C22724380_ScriptCanvas_RayCast_CollisionGroup")

    # 2) Retrieve and validate entities
    colliders = [EditorEntity.find_editor_entity(entity) for entity in COLLIDERS]
    Report.result(Tests.find_collider_entities, all(entity.id.isValid() for entity in colliders))

    raycasts = [EditorEntity.find_editor_entity(entity) for entity in RAYCASTS]
    Report.result(Tests.find_ray_cast_world_entities, all(entity.id.isValid() for entity in raycasts))

    script = EditorEntity.find_editor_entity("script")
    Report.result(Tests.find_script_entity, script.id.IsValid())

    # 3) Make sure that all the entities are in Active state
    all_entities = colliders + raycasts + [script]
    for entity in all_entities:
        entity.set_start_status("active")

    # 4) Start tracer to find expected lines
    with Tracer() as section_tracer:

        # All the expected lines are generated from Script canvas window in this test
        def find_expected_lines(line):
            for printInfo in section_tracer.prints:
                if printInfo.window == "Script Canvas" and printInfo.message.strip() == line:
                    return True

        # 5) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 6) Verify if all Raycasts worlds deactivated
        game_raycasts = [Raycast(i) for i in RAYCASTS]
        test_completed = helper.wait_for_condition(
            lambda: all(not entity.activated for entity in game_raycasts), RAYCAST_DEACTIVATE_TIMEOUT
        )
        Report.result(Tests.all_raycasts_deactivated, test_completed)

        # 7) Find the expected lines
        expected_lines_found = True
        for line in LogLines.expected_lines:
            line_found = helper.wait_for_condition(lambda: find_expected_lines(line), READLINE_TIMEOUT)
            if not line_found:
                Report.failure(f"Expected line not found : {line}")
            expected_lines_found = expected_lines_found and line_found

        Report.result(Tests.expected_lines_found, expected_lines_found)

        # 8) Disconnect handlers so they don't get called implicitly on level cleanup
        for entity in game_raycasts:
            entity.disconnect()

    # 9) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C22724380_ScriptCanvas_RayCast_CollisionGroup)
