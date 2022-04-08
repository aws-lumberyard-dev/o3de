#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This is a base Dockerfile to use for self-containing local or remote development environments
#
# Once docker is installed, build a local image with this command:
# `docker build /localDockerfilepath -t ubuntu-build:latest`
#
# To build using a local repo on disk, run this command:
# `docker run -it -v /localo3depath:/data/workspace/o3de -v /localbuildpath:/data/workspace/o3de/build -v /local3rdPartypath:/root/.o3de/3rdParty \
# --name build-o3de -d ubuntu-build:latest /bin/sh -c 'cd /data/workspace/o3de && python/python.sh -u scripts/build/ci_build.py --platform Linux --type profile'`
# 
# Attach to the running build to interact or view logs using this command:
# `docker attach build-o3de`

FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019

# Set longpath in registry
RUN Set-ItemProperty 'HKLM:\System\CurrentControlSet\Control\FileSystem' -Name 'LongPathsEnabled' -value 1

# Disable Windows Defender
RUN Set-MpPreference -DisableRealtimeMonitoring $true

# Disable Admin prompts
RUN New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name EnableLUA -PropertyType DWord -Value 0 -Force
RUN New-ItemProperty -Path HKLM:Software\Microsoft\Windows\CurrentVersion\policies\system -Name ConsentPromptBehaviorAdmin -PropertyType DWord -Value 0 -Force

# Install Chocolatey package manager
ENV ChocolateyUseWindowsCompression false
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iwr https://chocolatey.org/install.ps1 -UseBasicParsing | iex
RUN choco feature enable -n=allowGlobalConfirmation

# Install latest powershell (7.0.2)
RUN choco install -f powershell-core

# Set powershell 7 as default for every subsequent command
SHELL ["pwsh", "-command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

# Install source control apps 
RUN choco install -f git
RUN choco install -f git-lfs

# Install build and development tools
RUN git clone --no-checkout https://github.com/o3de/o3de.git .o3de && \
    cd .o3de && \
    git sparse-checkout init --cone && \
    git sparse-checkout set scripts/build/build_node && \
    cd scripts/build/build_node/Platform/Windows && \
    ./install-windows.bat

# Reload environment variables system file
RUN refreshenv

# Define the entry point for the docker container.
# This entry point starts the developer command prompt and launches the PowerShell shell.
ENTRYPOINT ["C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", "&&", "powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]