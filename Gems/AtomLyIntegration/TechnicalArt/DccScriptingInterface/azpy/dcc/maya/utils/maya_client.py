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

import logging
from azpy.shared.client_base import ClientBase
from PySide2.QtCore import Signal


_MODULENAME = 'azpy.dcc.maya.utils.maya_client'
_LOGGER = logging.getLogger(_MODULENAME)


class MayaClient(ClientBase):
    def echo(self, text):
        cmd = {
            'cmd': 'echo',
            'text': text
        }
        self.send(cmd)

    def set_title(self, title):
        cmd = {
            'cmd': 'set_title',
            'title': title
        }

        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None

    def run_script(self, target_path, script_arguments=None):
        cmd = {
            'cmd':   'run_script',
            'path': target_path,
            'arguments': script_arguments
        }

        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None
