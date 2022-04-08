<#
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
#>

# Install source control apps 
choco install -y git
choco install -y git-lfs

# Install Java (for Jenkins)
choco install corretto8jdk -y --ia INSTALLDIR="c:\jdk8" # Custom directory to handle cases where whitespace in the path is not quote wrapped

# Install CMake
choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'
