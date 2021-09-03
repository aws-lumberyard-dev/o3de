"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import core
from .aws_logexplorer_stack import AWSLogExplorerStack

class AWSLogExplorer(core.Construct):
    """
    Orchestrates setting up the AWS LogExplorer Stack(s)
    """
    def __init__(self,
                 scope: core.Construct,
                 id_: str,
                 project_name: str,
                 feature_name: str,
                 env: core.Environment) -> None:
        super().__init__(scope, id_)
        # Expectation is that you will only deploy this stack once and that the stack has regionless resources only
        stack_name = f'{project_name}-{feature_name}-{env.region}'

        # Deploy AWS LogExplorer Stack
        self._feature_stack = AWSLogExplorerStack(
            scope,
            stack_name,
            project_name=project_name,
            feature_name=feature_name,
            description=f'Contains resources for the AWS LogExplorer stack as part of the AWSLogExplorer project',
            tags={'LyAWSProject': project_name, 'LyAWSFeature': feature_name},
            env=env)

