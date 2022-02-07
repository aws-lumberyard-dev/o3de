@ECHO OFF
REM
REM Copyright (c) Contributors to the Open 3D Engine Project.
REM For complete copyright and license terms please see the LICENSE at the root of this distribution.
REM
REM SPDX-License-Identifier: Apache-2.0 OR MIT
REM
REM

pushd %~dp0%
CD %~dp0..
SET "$SCRIPT_DIR=%CD%"
popd

CALL python "%$SCRIPT_DIR%\o3de.py" %*
EXIT /b %ERRORLEVEL%
