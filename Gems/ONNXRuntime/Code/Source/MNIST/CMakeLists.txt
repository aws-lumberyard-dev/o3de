# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.15)

add_library(mnist mnist.cpp upng.cpp upng.h)

target_link_options(mnist PRIVATE "/SUBSYSTEM:CONSOLE")

target_compile_options(mnist PRIVATE /Zi /EHsc)