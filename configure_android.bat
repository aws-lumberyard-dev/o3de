@echo off 
set O3DE_ANDROID_SDK_PATH=D:\Android\Sdk 
set O3DE_ANDROID_NDK_VERSION=22.* 
set O3DE_ANDROID_SDK_API_LEVEL=29
set O3DE_ANDROID_ASSET_MODE=LOOSE 
set O3DE_ANDROID_DEPLOY_TYPE=BOTH
set O3DE_GRADLE_PATH=c:\ProgramData\chocolatey\bin\gradle.exe

SET O3DE_ENGINE_PATH=D:\git\o3de-dev
SET O3DE_PROJECT_PATH=D:\git\o3de-dev\AutomatedTesting
SET O3DE_BUILD_ROOT=D:\git\o3de-dev\build
SET O3DE_ANDROID_BUILD_PATH=%O3DE_BUILD_ROOT%\android
SET O3DE_3RDPARTY_PATH=D:\git\3rdparty

set JAVA_HOME=C:\Program Files\OpenJDK\jdk-21

REM %O3DE_BUILD_ROOT%\windows_vs2022\bin\profile\AssetProcessorBatch.exe --platforms=android --project-path %O3DE_PROJECT_PATH%

echo * Generating android project
echo.

CALL %O3DE_ENGINE_PATH%\python\python.cmd ^
 %O3DE_ENGINE_PATH%\cmake\Tools\Platform\Android\generate_android_project.py ^
 --engine-root %O3DE_ENGINE_PATH% ^
 --project-path %O3DE_PROJECT_PATH% ^
 --third-party-path %O3DE_3RDPARTY_PATH% ^
 --build-dir %O3DE_ANDROID_BUILD_PATH% ^
 --android-sdk-path %O3DE_ANDROID_SDK_PATH% ^
 --android-ndk-version %O3DE_ANDROID_NDK_VERSION% ^
 --android-sdk-platform %O3DE_ANDROID_SDK_API_LEVEL%  ^
 --enable-unity-build ^
 --include-apk-assets ^
 --asset-mode %O3DE_ANDROID_ASSET_MODE% ^
 --gradle-install-path "%O3DE_GRADLE_PATH%" ^
 --gradle-plugin-version 8.1.2


REM --overwrite-existing ^

echo * Building android project
echo.

pushd %O3DE_ANDROID_BUILD_PATH%
CALL gradlew --debug assembleProfile
echo Done
popd

REM goto end

echo * Deploying
CALL %O3DE_ENGINE_PATH%\python\python.cmd ^
 %O3DE_ENGINE_PATH%\cmake\Tools\Platform\Android\deploy_android.py ^
 --build-dir %O3DE_ANDROID_BUILD_PATH% ^
 --configuration profile ^
 --debug ^
 --clean -t %O3DE_ANDROID_DEPLOY_TYPE% ^
 --allow-unsigned 

:end
echo Done