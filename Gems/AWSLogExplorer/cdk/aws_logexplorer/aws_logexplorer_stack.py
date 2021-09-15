"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from aws_cdk import (
    core,
)

from .data_ingestion import DataIngestion

class AWSLogExplorerStack(core.Stack):

    def __init__(self, 
                 scope: core.Construct, 
                 id_: str, 
                 project_name: str, 
                 feature_name: str, 
                 **kwargs) -> None:
        super().__init__(scope, id_, **kwargs)

        self._data_ingestion = DataIngestion(self)
