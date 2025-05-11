

add_executable(tinystartstopdaemon tinystartstopdaemon.c)
add_executable(pipetest pipetest.cpp)

set_target_properties(tinystartstopdaemon PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH_ROOT})
set_target_properties(pipetest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH_ROOT})
