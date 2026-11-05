cmake_minimum_required(VERSION 3.22)

# Ensure CMake 4.x supports Raylib 5.0 CMake scripts (which declared VERSION 3.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(organism_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Common compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# -----------------------------------------------------------------------------
# Raylib FetchContent
# -----------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# -----------------------------------------------------------------------------
# Target 1: Headless Simulation Core Library (sim_core)
# -----------------------------------------------------------------------------
add_library(sim_core STATIC
    src/sim/GridMatrix.cpp
    src/sim/EntityManager.cpp
    src/sim/AIEngine.cpp
    src/sim/Simulation.cpp
)

target_include_directories(sim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(sim_core PUBLIC cxx_std_20)

# -----------------------------------------------------------------------------
# Target 2: Headless Test Suite Runner (sim_tests)
# -----------------------------------------------------------------------------
add_executable(sim_tests
    tests/test_main.cpp
    tests/test_grid.cpp
    tests/test_metabolism.cpp
    tests/test_ai_fsm.cpp
    tests/test_rules.cpp
)

target_include_directories(sim_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(sim_tests PRIVATE sim_core)

# -----------------------------------------------------------------------------
# Target 3: Interactive Raylib Application (organism_sim)
# -----------------------------------------------------------------------------
add_executable(organism_sim
    src/app/Renderer.cpp
    src/app/UI.cpp
    src/app/main.cpp
)

target_include_directories(organism_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(organism_sim PRIVATE
    sim_core
    raylib
)

if(WIN32)
    target_sources(organism_sim PRIVATE src/app/resource.rc)
endif()

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# -----------------------------------------------------------------------------
# Target 4: Clean & Rebuild Workflow Targets
# -----------------------------------------------------------------------------
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMENT "Cleaning all project build targets..."
)

add_custom_target(rebuild
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target organism_sim
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target sim_tests
    COMMENT "Executing full clean and rebuild of organism_sim and sim_tests..."
)

# -----------------------------------------------------------------------------
# Target 5: Installation Pipeline (GNU standard directories)
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

# 1. Standalone application binary
install(TARGETS organism_sim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. FreeDesktop .desktop application launcher
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/organism_sim.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# 3. Application icons for desktop shells & window managers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/icon.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/200x200/apps
    RENAME organism_sim.png
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/icon.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    RENAME organism_sim.png
)




