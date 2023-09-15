# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flash_disk)

option(IMAGE_SECONDARY "Builds the secondary image" OFF)

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/flash)

if(IMAGE_SECONDARY)
FILE(GLOB app_sources src/secondary.c)
else()
FILE(GLOB app_sources src/main.c)
endif()

target_sources(app PRIVATE ${app_sources})

if(IMAGE_SECONDARY)
message("Secondary image")
message(${CMAKE_CURRENT_SOURCE_DIR}/secondary.hex)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.hex ${CMAKE_CURRENT_SOURCE_DIR}/generated/secondary.hex
)
else()
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(app ${CMAKE_CURRENT_SOURCE_DIR}/generated/secondary.hex ${gen_dir}/secondary.inc)
endif()
