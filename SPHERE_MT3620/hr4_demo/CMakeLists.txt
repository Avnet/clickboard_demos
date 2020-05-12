#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project(HR4_Demo C) 

azsphere_configure_tools(TOOLS_REVISION "20.04")
azsphere_configure_api(TARGET_API_SET "5")

message( ${CMAKE_CURRENT_SOURCE_DIR} )


# set the path to the clickmodule library and Include the click library cmake file
set(click_path "../../clickmodules")
INCLUDE_DIRECTORIES( ${click_path}/LIGHTRANGER/core/inc )
INCLUDE_DIRECTORIES( ${click_path}/LIGHTRANGER/platform/inc )
INCLUDE_DIRECTORIES( ${click_path}/AIRQUALITY5 )
INCLUDE_DIRECTORIES( ${click_path}/BAROMETER )
INCLUDE_DIRECTORIES( ${click_path}/FLAME )
INCLUDE_DIRECTORIES( ${click_path}/HEARTRATE4 )
INCLUDE_DIRECTORIES( ${click_path}/LCDMINI )
INCLUDE_DIRECTORIES( ${click_path}/LSM6DSL )
INCLUDE_DIRECTORIES( ${click_path}/OLED-B )
INCLUDE_DIRECTORIES( ${click_path}/RELAY )
INCLUDE_DIRECTORIES( ${click_path}/TEMP_HUM )

# Create executable
add_executable(${PROJECT_NAME} main.c)
target_link_directories( ${PROJECT_NAME} PUBLIC ${click_path}/out/ARM-Debug )

target_link_libraries(${PROJECT_NAME} click applibs pthread gcc_s c)
#target_include_directories(${PROJECT_NAME} PUBLIC ${click_path}/clicklib.cmake)

target_compile_options(${PROJECT_NAME} PRIVATE -w)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../Hardware/mt3620_rdb" TARGET_DEFINITION "sample_hardware.json")
azsphere_target_add_image_package(${PROJECT_NAME})  
