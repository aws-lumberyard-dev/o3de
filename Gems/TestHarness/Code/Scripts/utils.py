"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Functions and constants used by both TestHarnessLauncher.py and conftest.py primarily
related to pre-testing set up and post-testing clean up.
"""
import json
import os
import psutil
import sys
import time

import test_tools.shared.waiter as waiter


def __find_engineroot():
    """
    Looks upward for engineroot.txt
    :return: path to the folder containing engineroot.txt
    """
    current = os.path.dirname(sys.executable)
    parent = os.path.dirname(current)
    while True:
        if current == parent:
            raise Exception("Could not find folder containing engineroot.txt above the python interpreter, is the interpreter bundled with Lumberyard being used?")
        if "engineroot.txt" in os.listdir(current):
            return current
        current = parent
        parent = os.path.dirname(current)


# Constants
# This path is fixed because currently there is no way for this python program
# to know a priori where the testharness_collection.json file lives.
__LOG_PATH = os.path.join("Cache", "SamplesProject", "pc", "user", "log", "testharnesslogs", "testharness_collection.json")
FILE_PATH = os.path.join(__find_engineroot(), __LOG_PATH)
SPEC = "all"


def get_file_path(launcher):
    """
    Single point of truth of where to find testharness_execution.json.

    :param launcher: the currently running launcher

    :return: file path to testharness_execution.json (type: string)
    """
    return os.path.join(launcher.workspace.release.paths.platform_cache(), 'user', 'log', 'TestHarnessLogs', 'testharness_execution.json')

def start_launcher(launcher):
    """
    Runs the launcher and checks if it's alive.

    :param launcher: the launcher to run
    """
    launcher.launch()
    assert launcher.is_alive(), "Launcher was closed unexpectedly"
    waiter.wait_for(lambda: check_for_listening_port(4600), timeout=120,
                                        exc=AssertionError('Port 4600 not listening.'))

def check_for_listening_port(port):
    """
    Checks if port is listening.
    
    :param port: port number to check on (type: int)

    :return boolean: True if port is listening.
    """
    port_listening = False
    for conn in psutil.net_connections():
        if 'port={}'.format(port) in str(conn):
            port_listening = True
    return port_listening

def send_ready_request(remote_console, launcher):
    """
    Sends a remote console command th_Ready to be received on the C++
    game runtime side, which will generate testharness_execution.json 
    and a ready signal timestamp being written to the json file by 
    TestHarnessSystemComponent. The existence of the json file and the 
    ready signal timestamp is checked to see if the game runtime is ready.
    
    :param remote_console: the currently running remote console
    :param launcher: the currently running launcher

    :return: path to testharness_execution.json (type: string)
    """
    file_path = get_file_path(launcher)
    remote_console.send_command('th_Ready')
    request_time = int(round(time.time() * 1000)) # in milliseconds
    waiter.wait_for(lambda: os.path.isfile(file_path) and 
                                        check_for_ready_signal(request_time, file_path),
                                        exc=AssertionError('Engine never gave ready signal.'))
    return file_path

def check_for_ready_signal(request_time, file_path):
    """
    Checks if the ready timestamp exists and if it is after
    the request time, to avoid a race condition where a previous
    ready signal (as represented by the ready timestamp) 
    appears before it is requested.

    :param request_time: the time in seconds (type: float)
    :param file_path: path to testharness_execution.json (type: string)

    :return boolean: True if the ready timestamp exists and is
    after the request time.
    """
    try:
        with open(file_path) as json_log:
            data = json.load(json_log)
    except IOError:
        return False
    try:
        ready_time = data["LevelReadySignal"]["ready_timestamp"]
    except KeyError:
        return False
    return (request_time < ready_time)

def launcher_teardown(launcher):
    """
    Teardown for a launcher.

    :param launcher: the currently running launcher
    """
    launcher.stop()
    launcher.ensure_stopped()
    launcher.teardown()

def remote_console_teardown(remote_console):
    """
    Teardown for a remote console.

    :param remote_console: the currently running remote console
    """
    if remote_console.connected:
        remote_console.stop()