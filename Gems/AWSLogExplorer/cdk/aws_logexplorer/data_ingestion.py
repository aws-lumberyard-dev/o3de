"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_s3 as s3,
    aws_kinesisfirehose as kinesisFirehose,
    aws_kinesisfirehose_destinations as destinations
)

class DataIngestion:
    """
    Create the S3 bucket and KinesisFirehose data stream to ingest metrics events.
    """
    
    def __init__(self, stack: core.Construct) -> None:
        self._stack = stack
        bucket = s3.Bucket(stack, "Bucket")
        self._input_stream = kinesisFirehose.DeliveryStream(
            self._stack,
            id='InputStream',
            destinations=[destinations.S3Bucket(bucket)],
            delivery_stream_name=f'{self._stack.stack_name}-InputStream'
        )

