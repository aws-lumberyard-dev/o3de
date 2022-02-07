#!/bin/sh

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Get the absolute path of this scripts folder
SCRIPT_DIR=$(cd `dirname $0` && pwd)

# Run s3sis.py and pass along the command
python "$SCRIPT_DIR/s3sis.py" $*
exit $?
