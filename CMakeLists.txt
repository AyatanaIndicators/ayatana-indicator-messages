project(ayatana-indicator-messages C CXX)
cmake_minimum_required(VERSION 3.13)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Globals

set(PROJECT_VERSION "22.2.0")
set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Options

option(ENABLE_TESTS "Enable all tests and checks" OFF)
option(ENABLE_COVERAGE "Enable coverage reports (includes enabling all tests and checks)" OFF)
option(ENABLE_WERROR "Treat all build warnings as errors" OFF)

if(ENABLE_COVERAGE)
    set(ENABLE_TESTS ON)
    set(CMAKE_BUILD_TYPE "Coverage")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(ENABLE_WERROR)
    add_definitions("-Werror")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions("-Weverything")
else()
    add_definitions("-Wall")
endif()

#  Check for prerequisites

include (GNUInstallDirs)
find_package(PkgConfig REQUIRED)
include(FindPkgConfig)

pkg_check_modules(
    PROJECT_DEPS
    REQUIRED
    glib-2.0>=2.36
    gio-unix-2.0>=2.36
    accountsservice
)

# Cppcheck

add_custom_target(
    cppcheck
    COMMAND
    cppcheck
    --enable=all
    -q
    --error-exitcode=2
    --inline-suppr
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Build

add_subdirectory(src)
add_subdirectory(libmessaging-menu)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(doc)

# Testing and coverage

if (ENABLE_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
    if (ENABLE_COVERAGE)
        find_package(CoverageReport)
        ENABLE_COVERAGE_REPORT(
            TARGETS "messaging-menu" "indicator-messages-service"
            TESTS ${COVERAGE_TEST_TARGETS}
            FILTER /usr/include ${CMAKE_BINARY_DIR}/*
        )
    endif ()
endif ()

# Display config info

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Unit tests: ${ENABLE_TESTS}")
message(STATUS "Build with -Werror: ${ENABLE_WERROR}")
