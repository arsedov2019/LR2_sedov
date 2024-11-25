cmake_minimum_required(VERSION 3.10)
project(keyboard_tracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

include_directories(/usr/include)
link_directories(/usr/lib)

add_executable(keyboard_tracker main.cpp)

include_directories(/usr/local/include/rxcpp)

if(UNIX AND NOT APPLE)
    target_link_libraries(keyboard_tracker PRIVATE ${X11_LIBRARIES} xkbcommon)
endif()
