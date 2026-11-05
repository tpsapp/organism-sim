cmake_minimum_required(VERSION 3.22)

# Fix for CMake 4.x compatibility with Raylib 5.0 CMakeLists.txt (which declared cmake_minimum_required(VERSION 3.0))
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(raylib_spike C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(spike_runner main.cpp)
target_link_libraries(spike_runner PRIVATE raylib)
