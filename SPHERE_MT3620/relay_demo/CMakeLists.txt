#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(relay_demo C)

azsphere_configure_tools(TOOLS_REVISION "20.10")
azsphere_configure_api(TARGET_API_SET "7")

# set the path to the clickmodule library and Include the click library cmake file
SET(click_path "../../clickmodules")
INCLUDE( ${click_path}/clicklib.cmake )
 
# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} click applibs pthread gcc_s c)

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})
