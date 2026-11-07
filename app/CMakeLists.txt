# Copyright (c) 2026 Mecha Systems
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.28.0)

# The build variants in snippets/ are used with `west build -S <name>`.
list(APPEND SNIPPET_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(comet_gamepad)

# Headers are included with their folder, for example "hid/gamepad_hid.h".
target_include_directories(app PRIVATE src)

target_sources(app PRIVATE src/main.c)

# One folder per part of the firmware. Optional parts are only built when their option is on.
add_subdirectory(src/gamepad)
add_subdirectory(src/hid)
add_subdirectory(src/usb)
add_subdirectory_ifdef(CONFIG_COMET_GAMEPAD_STATUS_LED src/status_led)
