"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    creation_undo = (
        "UNDO Entity creation success",
        "UNDO Entity creation failed")
    creation_redo = (
        "REDO Entity creation success",
        "REDO Entity creation failed")
    mesh_entity_creation = (
        "Mesh Entity successfully created",
        "Mesh Entity failed to be created")
    mesh_component_added = (
        "Entity has a Mesh component",
        "Entity failed to find Mesh component")
    mesh_asset_specified = (
        "Mesh asset set",
        "Mesh asset not set")
    enter_game_mode = (
        "Entered game mode",
        "Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "REDO deletion failed")


def AtomEditorComponents_Mesh_AddedToEntity():
    """
    Summary:
    Tests the Mesh component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Mesh entity with no components.
    2) Add a Mesh component to Mesh entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Specify the Mesh component asset
    6) Enter/Exit game mode.
    7) Test IsHidden.
    8) Test IsVisible.
    9) Delete Mesh entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

    :return: None
    """

    import os

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Actor entity with no components.
        actor_entity = EditorEntity.create_editor_entity(AtomComponentProperties.actor())
        Report.critical_result(Tests.mesh_entity_creation, mesh_entity.exists())

        # 2. Add a Actor component to Actor entity.
        actor_component = actor_entity.add_component(AtomComponentProperties.actor())
        Report.critical_result(
            Tests.actor_component_added,
            actor_entity.has_component(AtomComponentProperties.mesh()))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not actor_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, actor_entity.exists())

        # 5. Set Actor component asset property
        model_path = os.path.join('Objects', 'shaderball', 'shaderball_default_1m.azmodel')
        model = Asset.find_asset_by_path(model_path)
        actor_component.set_component_property_value(AtomComponentProperties.mesh('Actor Asset'), model.id)
        Report.result(Tests.mesh_asset_specified,
                      actor_component.get_component_property_value(AtomComponentProperties.mesh('Actor Asset')) == model.id)

        # 6. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 7. Test IsHidden.
        mesh_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, mesh_entity.is_hidden() is True)

        # 8. Test IsVisible.
        mesh_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, mesh_entity.is_visible() is True)

        # 9. Delete Mesh entity.
        mesh_entity.delete()
        Report.result(Tests.entity_deleted, not mesh_entity.exists())

        # 10. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, mesh_entity.exists())

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not mesh_entity.exists())

        # 12. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Mesh_AddedToEntity)


"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.asset as asset
import azlmbr.bus as bus
import azlmbr.camera
import azlmbr.entity as entity
import azlmbr.legacy.general as general
import azlmbr.math as math
import azlmbr.paths
import azlmbr.editor as editor

sys.path.append(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from Atom.atom_utils.screenshot_utils import ScreenshotHelper

SCREEN_WIDTH = 1280
SCREEN_HEIGHT = 720
DEGREE_RADIAN_FACTOR = 0.0174533

helper = EditorTestHelper(log_prefix="Test_Atom_BasicLevelSetup")


def run():
    """
    1. View -> Layouts -> Restore Default Layout, sets the viewport to ratio 16:9 @ 1280 x 720
    2. Runs console command r_DisplayInfo = 0
    3. Deletes all entities currently present in the level.
    4. Creates a "default_level" entity to hold all other entities, setting the translate values to x:0, y:0, z:0
    5. Adds a Grid component to the "default_level" & updates its Grid Spacing to 1.0m
    6. Adds a "global_skylight" entity to "default_level", attaching an HDRi Skybox w/ a Cubemap Texture.
    7. Adds a Global Skylight (IBL) component w/ diffuse image and specular image to "global_skylight" entity.
    8. Adds a "ground_plane" entity to "default_level", attaching a Mesh component & Material component.
    9. Adds a "directional_light" entity to "default_level" & adds a Directional Light component.
    10. Adds a "sphere" entity to "default_level" & adds a Mesh component with a Material component to it.
    11. Adds a "camera" entity to "default_level" & adds a Camera component with 80 degree FOV and Transform values:
        Translate - x:5.5m, y:-12.0m, z:9.0m
        Rotate - x:-27.0, y:-12.0, z:25.0
    12. Finally enters game mode, takes a screenshot, exits game mode, & saves the level.
    :return: None
    """
    def initial_viewport_setup(screen_width, screen_height):
        general.set_viewport_size(screen_width, screen_height)
        general.update_viewport()
        helper.wait_for_condition(
            function=lambda: helper.isclose(a=general.get_viewport_size().x, b=SCREEN_WIDTH, rel_tol=0.1)
            and helper.isclose(a=general.get_viewport_size().y, b=SCREEN_HEIGHT, rel_tol=0.1),
            timeout_in_seconds=4.0
        )
        result = helper.isclose(a=general.get_viewport_size().x, b=SCREEN_WIDTH, rel_tol=0.1) and helper.isclose(
            a=general.get_viewport_size().y, b=SCREEN_HEIGHT, rel_tol=0.1)
        general.log(general.get_viewport_size().x)
        general.log(general.get_viewport_size().y)
        general.log(general.get_viewport_size().z)
        general.log(f"Viewport is set to the expected size: {result}")
        general.run_console("r_DisplayInfo = 0")

    def after_level_load():
        """Function to call after creating/opening a level to ensure it loads."""
        # Give everything a second to initialize.
        general.idle_enable(True)
        general.idle_wait(1.0)
        general.update_viewport()
        general.idle_wait(0.5)  # half a second is more than enough for updating the viewport.

        # Close out problematic windows, FPS meters, and anti-aliasing.
        if general.is_helpers_shown():  # Turn off the helper gizmos if visible
            general.toggle_helpers()
            general.idle_wait(1.0)
        if general.is_pane_visible("Error Report"):  # Close Error Report windows that block focus.
            general.close_pane("Error Report")
        if general.is_pane_visible("Error Log"):  # Close Error Log windows that block focus.
            general.close_pane("Error Log")
        general.idle_wait(1.0)
        general.run_console("r_displayInfo=0")
        general.run_console("r_antialiasingmode=0")
        general.idle_wait(1.0)

        return True

    # Wait for Editor idle loop before executing Python hydra scripts.
    general.idle_enable(True)

    

    # Create global_skylight entity and set the properties
    global_skylight = hydra.Entity("global_skylight")
    global_skylight.create_entity(
        entity_position=math.Vector3(0.0, 0.0, 0.0),
        components=["HDRi Skybox", "Global Skylight (IBL)"],
        parent_id=default_level.id
    )
    global_skylight_image_asset_path = os.path.join(
        "LightingPresets", "greenwich_park_02_4k_iblskyboxcm_iblspecular.exr.streamingimage")
    global_skylight_image_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", global_skylight_image_asset_path, math.Uuid(), False)
    global_skylight.get_set_test(0, "Controller|Configuration|Cubemap Texture", global_skylight_image_asset)
    hydra.get_set_test(global_skylight, 1, "Controller|Configuration|Diffuse Image", global_skylight_image_asset)
    hydra.get_set_test(global_skylight, 1, "Controller|Configuration|Specular Image", global_skylight_image_asset)


    # Create MorphTarget entity and set the properties
    morph_target_entity = hydra.Entity("morphtarget")
    morph_target_entity.create_entity(
        entity_position=math.Vector3(0.0, 0.0, 1.0),
        components=["Material", "Actor", "Simple Motion"],
        parent_id=default_level.id
    )
    sphere_material_asset_path = os.path.join("Materials", "Presets", "PBR", "metal_brass_polished.azmaterial")
    sphere_material_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", sphere_material_asset_path, math.Uuid(), False)
    morph_target_entity.get_set_test(0, "Default Material|Material Asset", sphere_material_asset)

    # Set the actor
    morph_target_actor_path = os.path.join("Objects", "MorphTargets", "morphtargetactor.actor")
    morph_target_actor_asset  = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", morph_target_actor_path, math.Uuid(), False)
    morph_target_entity.get_set_test(1, "ActorAsset", morph_target_actor_asset)

    # Set the animation
    morph_target_animation_path = os.path.join("Objects", "MorphTargets", "morphtargetactor.motion")
    morph_target_animation_asset  = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", morph_target_animation_path, math.Uuid(), False)
    morph_target_entity.get_set_test(2, "Configuration|MotionAsset", morph_target_animation_asset)

    # Start the animation and set it to a specific point in time
    azlmbr.EditorSimpleMotionComponentRequestBus(azlmbr.bus.Event, 'SetPreviewInEditor', morph_target_entity.id, True)
    azlmbr.SimpleMotionComponentRequestBus(azlmbr.bus.Event, 'SetPlaySpeed', morph_target_entity.id, 0.0)
    azlmbr.SimpleMotionComponentRequestBus(azlmbr.bus.Event, 'PlayTime', morph_target_entity.id, 1.0)

    # Create camera component and set the properties
    camera_entity = hydra.Entity("camera")
    position = math.Vector3(5.5, -12.0, 9.0)
    camera_entity.create_entity(components=["Camera"], entity_position=position, parent_id=default_level.id)
    rotation = math.Vector3(
        DEGREE_RADIAN_FACTOR * -27.0, DEGREE_RADIAN_FACTOR * -12.0, DEGREE_RADIAN_FACTOR * 25.0
    )
    azlmbr.components.TransformBus(azlmbr.bus.Event, "SetLocalRotation", camera_entity.id, rotation)
    camera_entity.get_set_test(0, "Controller|Configuration|Field of view", 60.0)
    azlmbr.camera.EditorCameraViewRequestBus(azlmbr.bus.Event, "ToggleCameraAsActiveView", camera_entity.id)

    # Enter game mode, take screenshot, & exit game mode.
    general.enter_game_mode()
    general.idle_wait(1.0)
    helper.wait_for_condition(function=lambda: general.is_in_game_mode(), timeout_in_seconds=2.0)
    ScreenshotHelper(general.idle_wait_frames).capture_screenshot_blocking(f"{'AtomBasicLevelSetup'}.ppm")
    general.exit_game_mode()
    helper.wait_for_condition(function=lambda: not general.is_in_game_mode(), timeout_in_seconds=2.0)
    general.log("Basic level created")


if __name__ == "__main__":
    run()
