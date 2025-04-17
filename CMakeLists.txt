cmake_minimum_required(VERSION 3.10)
project(belvedere VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(ENABLE_DEBUG "Enable debug build" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Set build type
if(ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

# Enable testing
enable_testing()

# Detect OS and architecture
if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(ARCH "arm64")
    else()
        set(ARCH "x86_64")
    endif()
    set(OS "macos")
elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(ARCH "arm64")
    else()
        set(ARCH "x86_64")
    endif()
    set(OS "linux")
else()
    set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
    set(OS "unknown")
endif()

message(STATUS "Building for OS: ${OS}, Architecture: ${ARCH}")

# Find required packages
find_package(PkgConfig REQUIRED)

# Set library search paths based on OS and architecture
if(APPLE)
    if(ARCH STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()

    # macOS specific paths
    set(HIDAPI_SEARCH_PATHS
        "${HOMEBREW_PREFIX}/Cellar/hidapi/0.14.0/lib"
        "${HOMEBREW_PREFIX}/lib"
    )
    set(HIDAPI_INCLUDE_PATHS
        "${HOMEBREW_PREFIX}/Cellar/hidapi/0.14.0/include"
        "${HOMEBREW_PREFIX}/include"
    )
    set(LIBUV_SEARCH_PATHS
        "${HOMEBREW_PREFIX}/Cellar/libuv/1.49.2/lib"
        "${HOMEBREW_PREFIX}/lib"
    )
    set(LIBUV_INCLUDE_PATHS
        "${HOMEBREW_PREFIX}/Cellar/libuv/1.49.2/include"
        "${HOMEBREW_PREFIX}/include"
    )
    set(CUNIT_SEARCH_PATHS
        "/usr/local/Cellar/cunit/2.1-3/lib"
        "/usr/local/lib"
    )
    set(CUNIT_INCLUDE_PATHS
        "/usr/local/Cellar/cunit/2.1-3/include"
        "/usr/local/include"
    )
elseif(UNIX)
    # Linux paths
    set(HIDAPI_SEARCH_PATHS
        "/usr/lib"
        "/usr/local/lib"
        "/usr/lib/${ARCH}-linux-gnu"
    )
    set(HIDAPI_INCLUDE_PATHS
        "/usr/include"
        "/usr/local/include"
        "/usr/include/hidapi"
    )
    set(LIBUV_SEARCH_PATHS
        "/usr/lib"
        "/usr/local/lib"
        "/usr/lib/${ARCH}-linux-gnu"
    )
    set(LIBUV_INCLUDE_PATHS
        "/usr/include"
        "/usr/local/include"
    )
    set(CUNIT_SEARCH_PATHS
        "/usr/lib"
        "/usr/local/lib"
        "/usr/lib/${ARCH}-linux-gnu"
    )
    set(CUNIT_INCLUDE_PATHS
        "/usr/include"
        "/usr/local/include"
        "/usr/include/CUnit"
    )
endif()

# Find hidapi
find_library(HIDAPI_LIBRARY
    NAMES hidapi
    PATHS ${HIDAPI_SEARCH_PATHS}
    NO_DEFAULT_PATH
)
find_path(HIDAPI_INCLUDE_DIR
    NAMES hidapi/hidapi.h
    PATHS ${HIDAPI_INCLUDE_PATHS}
    NO_DEFAULT_PATH
)

if(NOT HIDAPI_LIBRARY)
    message(FATAL_ERROR "hidapi library not found")
endif()

if(NOT HIDAPI_INCLUDE_DIR)
    message(FATAL_ERROR "hidapi include directory not found")
endif()

# Find libuv
find_library(LIBUV_LIBRARY
    NAMES uv libuv
    PATHS ${LIBUV_SEARCH_PATHS}
    NO_DEFAULT_PATH
)
find_path(LIBUV_INCLUDE_DIR
    NAMES uv.h
    PATHS ${LIBUV_INCLUDE_PATHS}
    NO_DEFAULT_PATH
)

if(NOT LIBUV_LIBRARY)
    message(FATAL_ERROR "libuv library not found")
endif()

if(NOT LIBUV_INCLUDE_DIR)
    message(FATAL_ERROR "libuv include directory not found")
endif()

# Find CUnit
find_library(CUNIT_LIBRARY
    NAMES cunit
    PATHS ${CUNIT_SEARCH_PATHS}
    NO_DEFAULT_PATH
)
find_path(CUNIT_INCLUDE_DIR
    NAMES CUnit/CUnit.h
    PATHS ${CUNIT_INCLUDE_PATHS}
    NO_DEFAULT_PATH
)

if(NOT CUNIT_LIBRARY)
    message(FATAL_ERROR "CUnit library not found")
endif()

if(NOT CUNIT_INCLUDE_DIR)
    message(FATAL_ERROR "CUnit include directory not found")
endif()

# Add source files
set(SOURCES
    src/belvedere.c
    src/config.c
    src/debug.c
    src/hid_manager.c
)

# Add header files
set(HEADERS
    include/config.h
    include/debug.h
    include/hid_manager.h
)

# Create executable
add_executable(belvedere ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(belvedere PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${HIDAPI_INCLUDE_DIR}
    ${LIBUV_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(belvedere PRIVATE
    ${HIDAPI_LIBRARY}
    ${LIBUV_LIBRARY}
)

# Install executable
install(TARGETS belvedere
    RUNTIME DESTINATION bin
)

# Install configuration file according to XDG Base Directory Specification
if(UNIX)
    # Get user's home directory
    execute_process(
        COMMAND echo $HOME
        OUTPUT_VARIABLE USER_HOME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Set XDG config directory
    set(XDG_CONFIG_HOME "$ENV{XDG_CONFIG_HOME}")
    if("${XDG_CONFIG_HOME}" STREQUAL "")
        set(XDG_CONFIG_HOME "${USER_HOME}/.config")
    endif()

    # Install default configuration file to system directory
    install(FILES etc/default_config.ini
        DESTINATION etc/belvedere
    )

    # Install utility scripts
    install(PROGRAMS scripts/reload_config.sh
        DESTINATION bin
    )

    # Create post-install script to ensure directory exists
    file(WRITE ${CMAKE_BINARY_DIR}/post_install.sh
        "#!/bin/sh\n"
        "mkdir -p ${XDG_CONFIG_HOME}/belvedere\n"
        "if [ ! -f ${XDG_CONFIG_HOME}/belvedere/config ]; then\n"
        "    cp ${CMAKE_INSTALL_PREFIX}/etc/belvedere/default_config.ini ${XDG_CONFIG_HOME}/belvedere/config\n"
        "fi\n"
    )
    execute_process(COMMAND chmod +x ${CMAKE_BINARY_DIR}/post_install.sh)

    # Add custom target to run post-install script
    install(CODE "execute_process(COMMAND ${CMAKE_BINARY_DIR}/post_install.sh)")
else()
    # Fallback for non-UNIX systems
    install(FILES etc/default_config.ini
        DESTINATION etc/belvedere
        RENAME config
    )

    # Install utility scripts for non-UNIX systems
    install(PROGRAMS scripts/reload_config.sh
        DESTINATION bin
    )
endif()

# Testing
if(BUILD_TESTS)
    # Add test executable
    add_executable(test_config tests/test_config.c src/config.c src/debug.c)
    target_link_libraries(test_config PRIVATE
        ${CMAKE_DL_LIBS}
        ${CUNIT_LIBRARY}
    )
    target_include_directories(test_config PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CUNIT_INCLUDE_DIR}
    )

    add_executable(test_hid_manager tests/test_hid_manager.c src/hid_manager.c src/config.c src/debug.c)
    target_link_libraries(test_hid_manager PRIVATE
        ${CMAKE_DL_LIBS}
        ${HIDAPI_LIBRARY}
        ${LIBUV_LIBRARY}
        ${CUNIT_LIBRARY}
    )
    target_include_directories(test_hid_manager PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${HIDAPI_INCLUDE_DIR}
        ${LIBUV_INCLUDE_DIR}
        ${CUNIT_INCLUDE_DIR}
    )

    # Add test targets to CTest
    add_test(NAME test_config COMMAND test_config)
    add_test(NAME test_hid_manager COMMAND test_hid_manager)

    # Add custom target that runs all tests
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_config test_hid_manager
        COMMENT "Running all tests..."
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Code coverage
    if(ENABLE_COVERAGE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)

        if(GCOV_PATH AND LCOV_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -c -i -d . -o coverage.base
                COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -c -d . -o coverage.run
                COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -a coverage.base -a coverage.run -o coverage.info
                COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} -r coverage.info '/usr/*' -o coverage.info
                COMMAND genhtml -o coverage_html coverage.info
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating coverage report"
            )
        endif()
    endif()
endif()