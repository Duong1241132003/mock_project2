cmake_minimum_required(VERSION 3.15)
project(MediaPlayerApp VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/inc)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)

# SDL2_mixer via PkgConfig is more reliable on some systems
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

# TagLib
pkg_check_modules(TAGLIB REQUIRED taglib)




# Source files (excluding video-related files)
file(GLOB_RECURSE SOURCES 
    "src/models/*.cpp"
    "src/controllers/*.cpp"
    "src/views/*.cpp"
    "src/services/*.cpp"
    "src/repositories/*.cpp"
    "src/utils/*.cpp"
    "src/core/*.cpp"
    "src/config/*.cpp"
    "src/ui/*.cpp"
)





# Main executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
    src/main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2
    SDL2_ttf
    ${SDL2_MIXER_LIBRARIES}
    ${TAGLIB_LIBRARIES}


    pthread
    stdc++fs
)

# Include directories for external libraries
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}


)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Create necessary directories

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data/playlists)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data/library)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data/history)

# Testing (optional)
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    
    find_package(GTest REQUIRED)
    
    # Coverage flags will be added to RunTests target directly
    
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    
    add_executable(RunTests ${TEST_SOURCES} ${SOURCES})
    
    target_include_directories(RunTests PRIVATE
        ${PROJECT_SOURCE_DIR}/tests
    )
    
    target_link_libraries(RunTests
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        SDL2::SDL2
        SDL2_ttf
        ${SDL2_MIXER_LIBRARIES}
        ${TAGLIB_LIBRARIES}

        pthread
        stdc++fs
    )
    
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(RunTests PRIVATE --coverage -O0 -g)
        target_link_options(RunTests PRIVATE --coverage)
    endif()

    add_test(NAME RunTests COMMAND RunTests)
    
    # Add coverage target
    # Add coverage target using lcov with branch coverage enabled
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage_report
        COMMAND lcov --directory . --zerocounters
        COMMAND ./RunTests
        COMMAND lcov --directory ${CMAKE_BINARY_DIR}/CMakeFiles/RunTests.dir/src/controllers --capture --output-file coverage_controllers.info --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1
        COMMAND lcov --directory ${CMAKE_BINARY_DIR}/CMakeFiles/RunTests.dir/src/services --capture --output-file coverage_services.info --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1
        COMMAND lcov --directory ${CMAKE_BINARY_DIR}/CMakeFiles/RunTests.dir/src/models --capture --output-file coverage_models.info --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1
        COMMAND lcov --directory ${CMAKE_BINARY_DIR}/CMakeFiles/RunTests.dir/src/repositories --capture --output-file coverage_repositories.info --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1
        COMMAND lcov --directory ${CMAKE_BINARY_DIR}/CMakeFiles/RunTests.dir/src/config --capture --output-file coverage_config.info --ignore-errors mismatch,gcov --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1
        COMMAND lcov --add-tracefile coverage_controllers.info --add-tracefile coverage_services.info --add-tracefile coverage_models.info --add-tracefile coverage_repositories.info --add-tracefile coverage_config.info --output-file coverage.info --rc lcov_branch_coverage=1 --ignore-errors mismatch
        COMMAND lcov --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' '*.h' '*/inc/*' '*/src/views/*' '*/src/ui/*' '*/src/utils/*' '*/src/core/*' '*/AudioPlaybackEngine.cpp' '*/SerialCommunication.cpp' --output-file coverage.info --ignore-errors mismatch --rc geninfo_unexecuted_blocks=1 --rc lcov_branch_coverage=1 --ignore-errors unused
        COMMAND genhtml coverage.info --output-directory coverage_report --ignore-errors mismatch --branch-coverage
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report with lcov (including branch coverage)..."
    )
    add_dependencies(coverage RunTests)
endif()

# Print configuration
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Media Player Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "========================================")
message(STATUS "")
