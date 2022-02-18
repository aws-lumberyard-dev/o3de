"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.bus
import azlmbr.editor
import azlmbr.legacy.general
import sys
import os
from datetime import datetime


# This test was relying on each command it ran from Python
# to trigger a message to print out to editor.log.
# Right now there's an issue where not all messages show up
# in editor.log. This print to a file here is to demonstrate
# that this inner python script is running, and running correctly.
# When the test is run, if it passes or fails these messages all still show up.
def debug_print(message):
    file_object = open("C:/Logs/run_editor.txt", 'a')

    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    file_object.write(f"{current_time}: {message}" + "\n")
    file_object.close()


debug_print("Start")
# Print out the passed in bundle_path, so the outer test can verify this was sent in correctly
bundle_path = sys.argv[1]
print('Bundle mode test running with path {}'.format(sys.argv[1]))

debug_print(f"bundle_path: {bundle_path}")

# Turn on bundle mode. This will trigger some printouts that the outer test logic will validate.
azlmbr.legacy.general.set_cvar_integer("sys_report_files_not_found_in_paks", 1)
azlmbr.legacy.general.run_console(f"loadbundles {bundle_path}")

debug_print("run_console")

azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'ExitNoPrompt')

debug_print("Finish")
