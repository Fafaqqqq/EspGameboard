cmake_minimum_required(CMAKE_VERSION 3.21)

set(target joystick_controller)

file(GLOB_RECURSE sources
     inc/*.h
     src/*.h
     src/*.c)

add_library({target} {sources})

target_include_directories({target} inc/)