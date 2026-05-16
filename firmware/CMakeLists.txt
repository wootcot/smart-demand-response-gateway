# Minimum CMake version required by ESP-IDF v5.x build system
cmake_minimum_required(VERSION 3.16)

# Include the ESP-IDF CMake toolchain which provides idf_component_register,
# partition table handling, and all ESP-IDF build infrastructure
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project declaration — name used for the output binary artifact
project(nepal_grid_peak_load_controller)
