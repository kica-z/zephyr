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

set(SECONDARY_IMAGE_HEX ${CMAKE_CURRENT_SOURCE_DIR}/generated/secondary.hex)
if(IMAGE_SECONDARY)
message("Secondary image")
set(ZEPHYR_OUTPUT_HEX ${CMAKE_CURRENT_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME})
add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E copy ${ZEPHYR_OUTPUT_HEX} ${SECONDARY_IMAGE_HEX}
    OUTPUT  ${SECONDARY_IMAGE_HEX}
    DEPENDS ${ZEPHYR_OUTPUT_HEX}
    COMMENT "Running post-build copying step..."
)
add_custom_target(
    post_bin ALL
    DEPENDS ${SECONDARY_IMAGE_HEX}
)
else()
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(app ${SECONDARY_IMAGE_HEX} ${gen_dir}/secondary.inc)
endif()
