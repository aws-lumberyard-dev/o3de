<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

choco install -y visualstudio2019buildtools --version=16.8.2 --package-parameters "--config .\vs2019bt.vsconfig" -y
choco install -y visualstudio2017buildtools --version=15.9.29 --package-parameters "--config .\vs2017bt.vsconfig" -y

Write-Host "Installing downstream dependancies"
choco install -y dotnet3.5 
choco install -y windows-sdk-10.1

# Install Windows Installer XML toolkit (WiX)
choco install -y wixtoolset