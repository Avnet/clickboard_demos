#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project(HR4_Demo C)

azsphere_configure_tools(TOOLS_REVISION "20.04")
azsphere_configure_api(TARGET_API_SET "5") 

# set the path to the clickmodule library and Include the click library cmake file
set(click_path "../../clickmodules/HEARTRATE4") 
set(click_lib_path "C:/Users/051520/source/repos/AvnetPublicProjects/clickboard_demos/clickmodules/HEARTRATE4")

# Create executable 
add_executable(${PROJECT_NAME} main.c algorithm_by_RF.c max30102.c) 
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)
target_include_directories(${PROJECT_NAME} PUBLIC ${click_path})
target_compile_options(${PROJECT_NAME} PRIVATE -w)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${projectDir}..\\Hardware\\mt3620_rdb" TARGET_DEFINITION "sample_hardware.json")
azsphere_target_add_image_package(${PROJECT_NAME}) 