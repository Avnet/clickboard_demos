#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(click_demo C)

# set the path to the clickmodule library and Include the click library cmake file
SET(click_path "../../clickmodules")
INCLUDE( ${click_path}/clicklib.cmake )

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c epoll_timerfd_utilities.c oled_test.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} click applibs pthread gcc_s c)

target_compile_options(${PROJECT_NAME} PRIVATE -w)

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")


