cmake_minimum_required(VERSION 3.20)
project(tiny_systemd)
set(EXECUTABLE_OUTPUT_PATH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/z_output)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
add_subdirectory(src)
add_subdirectory(tool)


