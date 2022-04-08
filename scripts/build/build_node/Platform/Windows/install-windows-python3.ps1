<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

choco install -y python3 --version=3.7.12

# Ensure Python paths are set
[Environment]::SetEnvironmentVariable(
    "Path",
    [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::Machine) + ";C:\Python37;C:\Python37\Scripts",
    [EnvironmentVariableTarget]::Machine)

Write-Host "Installing packages"
python -m pip install -r .\requirements.txt