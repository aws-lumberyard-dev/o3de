"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID : C19723162
Test Case Title : Verify Shape Cast nodes in script canvas for Shape Colliders
URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/19723162
"""


# fmt: off
class Tests:
    enter_game_mode                = ("Entered game mode",                      "Failed to enter game mode")
    ball_0_found                   = ("Ball_0 entity is found",                 "Ball_0 entity is not found")
    ball_1_found                   = ("Ball_1 entity is found",                 "Ball_1 entity is not found")
    notification_entity_0_found    = ("Notification_Entity_0 entity found",     "Notification_Entity_0 entity invalid")
    notification_entity_1_found    = ("Notification_Entity_1 entity found",     "Notification_Entity_1 entity invalid")
    notification_entity_2_found    = ("Notification_Entity_2 entity found",     "Notification_Entity_2 entity invalid")
    ball_0_gravity                 = ("Ball_0 gravity is disabled",             "Ball_0 gravity is enabled")
    ball_1_gravity                 = ("Ball_1 gravity is disabled",             "Ball_1 gravity is enabled")
    notification_entity_0_gravity  = ("Notification_Entity_0 gravity disabled", "Notification_Entity_0 gravity enabled")
    notification_entity_1_gravity  = ("Notification_Entity_1 gravity disabled", "Notification_Entity_1 gravity enabled")
    notification_entity_2_gravity  = ("Notification_Entity_2 gravity disabled", "Notification_Entity_2 gravity enabled")
    script_canvas_translation_node = ("Script canvas through translation node", "Script canvas failed translation node")
    script_canvas_sphere_cast_node = ("Script canvas through sphere cast node", "Script canvas failed sphere cast node")
    script_canvas_draw_sphere_node = ("Script canvas through draw sphere node", "Script canvas failed draw sphere node")
# fmt: on


def C19723162_ScriptCanvas_ShapeColiderVerifyShapeCastNode():
    """
    Summary:
     Verify Shape Cast nodes in script canvas for Shape Colliders

    Level Description:
     Ball_0 (entity) - Starts stationary due to disabled gravity, located on -y axis from Ball_1, has components
        Sphere shape collider, Rigid Body, Sphere Shape and Script Canvas
     Ball_1 (entity) - Starts stationary due to disabled gravity, located on +y axis from Ball_0, has components
        Sphere shape collider, Rigid Body, Sphere Shape and Script Canvas
     Notification_Entity_0 (entity)- Start stationary with gravity disabled, has rigid body and Sphere shape components
     Notification_Entity_1 (entity)- Start stationary with gravity disabled, has rigid body and Sphere shape components
     Notification_Entity_2 (entity)- Start stationary with gravity disabled, has rigid body and Sphere shape components
     Script Canvas - Creates a sphere cast from Ball_0 to Ball_1 and draws the sphere in. As the script progresses
        through the translation, sphere cast, and sphere draw nodes, it enables gravity on the spheres in order to
        show that each node has been passed.

    Expected Behavior:
     A sphere will be drawn into ball_1 from ball_0. Gravity gets enabled in three notification entities.

    Test Steps:
     1) Open level
     2) Enter Game Mode
     3) Create game entity objects to validate game entities in level
     4) Verify entities and check gravity
     5) Wait for conditions or timeout
     6) Report results

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

    # Lumberyard imports
    import azlmbr.legacy.general as general
    import azlmbr.bus

    # Helper Functions
    class Entity:
        # Constants
        GRAVITY_WAIT_TIMEOUT = 1  # Time in seconds

        def __init__(self, name, expected_initial_velocity=None, expected_final_velocity=None):
            self.id = general.find_game_entity(name)
            self.name = name
            self.gravity_was_enabled = False
            self.handler = None
            # 4) Verify entities and check gravity
            try:
                self.found_test = Tests.__dict__[self.name.lower() + "_found"]
                self.gravity_disabled_test = Tests.__dict__[self.name.lower() + "_gravity"]
            except Exception as e:
                Report.info(f"Could not find {self.name.lower() + '_found'} test in class : Tests")
                Report.info(e)
                raise ValueError
            Report.critical_result(self.found_test, self.id.isValid())
            Report.critical_result(self.gravity_disabled_test, not self.check_gravity_enabled())

        def check_gravity_enabled(self):
            self.gravity_was_enabled = azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)
            return self.gravity_was_enabled

        def wait_for_gravity_enabled(self):
            helper.wait_for_condition(self.check_gravity_enabled, Entity.GRAVITY_WAIT_TIMEOUT)

    # Main Script
    helper.init_idle()
    # 1) Open level
    helper.open_level("Physics", "C19723162_ScriptCanvas_ShapeColiderVerifyShapeCastNode")

    # 2) Enter Game Mode
    helper.enter_game_mode(Tests.enter_game_mode)

    # 3) Create game entity objects to validate game entities in level
    Entity("Ball_0")
    Entity("Ball_1")
    notification_entity_0 = Entity("Notification_Entity_0")
    notification_entity_1 = Entity("Notification_Entity_1")
    notification_entity_2 = Entity("Notification_Entity_2")

    # 5) Wait for conditions or timeout
    notification_list = [notification_entity_0, notification_entity_1, notification_entity_2]
    for entity in notification_list:
        entity.wait_for_gravity_enabled()

    # 6) Report results
    Report.result(Tests.script_canvas_translation_node, notification_entity_0.gravity_was_enabled)
    Report.result(Tests.script_canvas_sphere_cast_node, notification_entity_1.gravity_was_enabled)
    Report.result(Tests.script_canvas_draw_sphere_node, notification_entity_2.gravity_was_enabled)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C19723162_ScriptCanvas_ShapeColiderVerifyShapeCastNode)
