"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


# Test Case ID    : C19723159
# Test Case Title : Verify Script Canvas Collision Events for Shape Colliders


# fmt: off
class Tests:
    enter_game_mode             = ("Entered game mode",             "Failed to enter game mode")
    terrain_entity_found        = ("PhysX Terrain entity is found", "Failed to find PhysX Terrain entity")
    sphere_entity_found         = ("Sphere entity found",           "Failed to find Sphere entity")
    begin_signal_entity_found   = ("Begin Signal entity found",     "Failed to find Begin Signal entity")
    persist_signal_entity_found = ("Persist Signal entity found",   "Failed to find Persist Signal entity")
    end_signal_entity_found     = ("End Signal entity found",       "Failed to find End Signal entity")
    sphere_is_above_terrain     = ("Sphere is above the terrain",   "Sphere is not above the terrain")
    sphere_gravity_enabled      = ("Gravity is enabled on Sphere",  "Failed to enable Gravity on Sphere")
    sphere_started_bouncing     = ("Sphere started bouncing",       "Sphere failed to start bouncing")
    sphere_stopped_bouncing     = ("Sphere stopped bouncing",       "Sphere failed stop bouncing")
    event_records_match         = ("Event records match",           "Event records do not match")
    exit_game_mode              = ("Exited game mode",              "Failed to exit game mode")
# fmt: on


def C19723159_ScriptCanvas_ShapeColliderCollisionEvents():
    """
    Summary:
     This script runs an automated test to verify that the Script Canvas nodes "On Collision Begin", "On Collision
     Persist", and "On Collision End" will function as intended for the entity to which their Script Canvas is attached.

    Level Description:
     A sphere (entity: Sphere) is above a terrain (entity: PhysX Terrain). The sphere has a PhysX Rigid Body, a PhysX
     Shape Collider with shape Sphere, and gravity enabled.
     The sphere has a Script Canvas attached to it which will toggle the activation of three signal entities 
     (entity: Begin Signal), (entity: Persist Signal), and (entity: End Signal).
     Begin Signal's activation will toggle (switch from activated to deactivated or vice versa) when a collision begins
     with the sphere, Persist Signal's activation will toggle when collision persists with the sphere, and End Signal's
     activation will toggle when a collision ends with the sphere.

    Expected behavior:
     The sphere will fall toward and collide with the terrain. The sphere will bounce until it comes to rest. The Script
     Canvas will cause the signal entities to activate and deactivate in the same pattern as the collision events which
     occur on the sphere.

    Test Steps:
     1) Open level and enter game mode
     2) Retrieve and validate entities
     3) Check that the sphere is above the terrain
     4) Check that gravity is enabled on the sphere
     5) Wait for the initial collision between the sphere and the terrain or timeout
     6) Wait for the sphere to stop bouncing
     7 Check that the event records match
     8) Exit game mode and close editor

    Note:
     - This test file must be called from the Lumberyard Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Helper Imports

    import ImportPathHelper as imports

    imports.init()

    import azlmbr.legacy.general as general
    import azlmbr.bus
    import azlmbr.components
    import azlmbr.entity
    import azlmbr.physics
    from utils import Report
    from utils import TestHelper as helper

    # Constants
    TIME_OUT_SECONDS = 3.0
    TERRAIN_START_Z = 32.0
    SPHERE_RADIUS = 1.0

    class Entity:
        def __init__(self, name, found_valid_test):
            self.name = name
            self.id = general.find_game_entity(name)
            self.found_valid_test = found_valid_test

    class Sphere(Entity):
        def __init__(self, name, found_valid_test, event_records_match_test):
            Entity.__init__(self, name, found_valid_test)
            self.event_records_match_test = event_records_match_test
            self.collided = False
            self.stopped_bouncing = False
            self.collision_event_record = []
            self.script_canvas_event_record = []

            # Set up collision notification handler
            self.handler = azlmbr.physics.CollisionNotificationBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnCollisionBegin", self.on_collision_begin)
            self.handler.add_callback("OnCollisionPersist", self.on_collision_persist)
            self.handler.add_callback("OnCollisionEnd", self.on_collision_end)

        def get_z_position(self):
            z_position = azlmbr.components.TransformBus(azlmbr.bus.Event, "GetWorldZ", self.id)
            Report.info(f"{self.name}'s z-position: {z_position}")
            return z_position

        def is_gravity_enabled(self):
            return azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsGravityEnabled", self.id)

        # Set up reporting of the event records and whether they match
        def match_event_records(self):
            Report.info(f"{self.name} collision event record: {self.collision_event_record}")
            Report.info(f"Script Canvas event record: {self.script_canvas_event_record}")
            return self.collision_event_record == self.script_canvas_event_record

        # Set up collision event detection and update collision event record
        def on_collision(self, event, other_id):
            if not self.collided:
                self.collided = True
            other_name = azlmbr.entity.GameEntityContextRequestBus(azlmbr.bus.Broadcast, "GetEntityName", other_id)
            Report.info("f{self.name} collision {event}s with {other_name}")
            self.collision_event_record.append(event)

        def on_collision_begin(self, args):
            self.on_collision("begin", args[0])

        def on_collision_persist(self, args):
            self.on_collision("persist", args[0])

        def on_collision_end(self, args):
            self.on_collision("end", args[0])

        # Set up detection of the sphere coming to rest
        def bouncing_stopped(self):
            if not azlmbr.physics.RigidBodyRequestBus(azlmbr.bus.Event, "IsAwake", self.id):
                self.stopped_bouncing = True
            return self.stopped_bouncing

    class SignalEntity(Entity):
        def __init__(self, name, found_valid_test, monitored_entity, event):
            Entity.__init__(self, name, found_valid_test)
            self.monitored_entity = monitored_entity
            self.event = event

            # Set up activation and deactivation notification handler
            self.handler = azlmbr.entity.EntityBusHandler()
            self.handler.connect(self.id)
            self.handler.add_callback("OnEntityActivated", self.on_entity_activated)
            self.handler.add_callback("OnEntityDeactivated", self.on_entity_deactivated)

        # Set up activation and deactivation detection and update Script Canvas event record
        def on_entity_activated(self, args):
            self.monitored_entity.script_canvas_event_record.append(self.event)

        def on_entity_deactivated(self, args):
            self.monitored_entity.script_canvas_event_record.append(self.event)

    # 1) Open level and enter game mode
    helper.init_idle()
    helper.open_level("Physics", "C19723159_ScriptCanvas_ShapeColliderCollisionEvents")
    helper.enter_game_mode(Tests.enter_game_mode)

    # 2) Retrieve and validate entities
    terrain = Entity("PhysX Terrain", Tests.terrain_entity_found)
    sphere = Sphere("Sphere", Tests.sphere_entity_found, Tests.event_records_match)
    begin_signal = SignalEntity("Begin Signal", Tests.begin_signal_entity_found, sphere, "begin")
    persist_signal = SignalEntity("Persist Signal", Tests.persist_signal_entity_found, sphere, "persist")
    end_signal = SignalEntity("End Signal", Tests.end_signal_entity_found, sphere, "end")

    entities = [terrain, sphere, begin_signal, persist_signal, end_signal]
    for entity in entities:
        Report.critical_result(entity.found_valid_test, entity.id.IsValid())

    # 3) Check that the sphere is above the terrain
    Report.critical_result(Tests.sphere_is_above_terrain, sphere.get_z_position() - SPHERE_RADIUS > TERRAIN_START_Z)

    # 4) Check that gravity is enabled on the sphere
    Report.critical_result(Tests.sphere_gravity_enabled, sphere.is_gravity_enabled())

    # 5) Wait for the initial collision between the sphere and the terrain or timeout
    helper.wait_for_condition(lambda: sphere.collided, TIME_OUT_SECONDS)
    Report.critical_result(Tests.sphere_started_bouncing, sphere.collided)

    # 6) Wait for the sphere to stop bouncing
    helper.wait_for_condition(sphere.bouncing_stopped, TIME_OUT_SECONDS)
    Report.result(Tests.sphere_stopped_bouncing, sphere.stopped_bouncing)

    # 7) Check that the event records match
    Report.result(Tests.event_records_match, sphere.match_event_records())

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()

    from utils import Report

    Report.start_test(C19723159_ScriptCanvas_ShapeColliderCollisionEvents)
