project(tinysystem)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Network DBus REQUIRED)

file(GLOB SRCS
        *.cpp
        daemon/*.cpp
)

link_libraries(
        Qt5::Core
        Qt5::Network
        Qt5::DBus
)

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(tinysystemd ${SRCS} cmd/tiny_systemd.cpp)
add_executable(tinysystemctl ${SRCS} cmd/tiny_system_ctl.cpp)


set_target_properties(tinysystemd PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH_ROOT})
set_target_properties(tinysystemctl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH_ROOT})