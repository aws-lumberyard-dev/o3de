"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Configurations for the fleets to deploy.
# Please fill or modify the fleet configuration fields below before deploying the CDK application.
# Check the following document for the complete list of available fleet configurations.
# https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/AWS_GameLift.html
FLEET_CONFIGURATIONS = [
    {
        # An alias for an Amazon GameLift fleet destination
        'alias_configuration': {
            # A descriptive label that is associated with an alias. Alias names do not need to be unique.
            'name': '',
            # A type of routing strategy for the GameLift fleet alias if exists.
            'routing_strategy': {
                # The message text to be used with a terminal routing strategy.
                # If you specify TERMINAL for the Type property, you must specify this property.
                'message': '',
                # A type of routing strategy.
                'type': 'SIMPLE'
            }
        },
        # Information about a game server build that is installed and run on instances in an Amazon GameLift fleet.
        'build_configuration': {
            # A unique identifier for a build to be deployed on the new fleet.
            'build_id': '',
            # The disk location of the local build file(s).
            'build_path': '',
            # The operating system that the game server binaries are built to run on.
            'operating_system': 'WINDOWS_2012'
        },
        # Information about the use of a TLS/SSL certificate for a fleet.
        'certificate_configuration': {
            # Indicates whether a TLS/SSL certificate is generated for the fleet.
            'certificate_type': 'DISABLED',
        },
        # A range of IP addresses and port settings that allow inbound traffic
        # to connect to server processes on an Amazon GameLift server.
        'ec2_inbound_permissions': [
            {
                'from_port': 30090,
                'to_port': 30090,
                'ip_range': '',
                'protocol': 'UDP'
            }
        ],
        # The GameLift-supported EC2 instance type to use for all fleet instances.
        'ec2_instance_type': 'c5.large',
        # Indicates whether to use On-Demand or Spot instances for this fleet.
        'fleet_type': 'ON_DEMAND',
        # A game session protection policy to apply to all game sessions hosted on instances in this fleet.
        'new_game_session_protection_policy': 'NoProtection',
        # A policy that limits the number of game sessions that an individual player
        # can create on instances in this fleet within a specified span of time.
        'resource_creation_limit_policy': {
            'new_game_sessions_per_creator': 3,
            'policy_period_in_minutes': 15
        },
        # Instructions for launching server processes on each instance in the fleet.
        'runtime_configuration': {
            'game_session_activation_timeout_seconds': 300,
            'max_concurrent_game_session_activations': 2,
            'server_processes': [
                {
                    'concurrent_executions': 1,
                    'launch_path': '',
                    'parameters': ''
                }
            ]
        }
    }
]
