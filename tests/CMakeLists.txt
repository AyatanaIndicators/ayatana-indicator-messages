# libindicator-messages-service

find_package(GMock)

set(
    HEADERS
    ${CMAKE_SOURCE_DIR}/src/gactionmuxer.h
    ${CMAKE_SOURCE_DIR}/src/dbus-data.h
)

set(
    SOURCES
    ${CMAKE_SOURCE_DIR}/src/gactionmuxer.c
)

set(
    HEADERS_GEN
    ${CMAKE_BINARY_DIR}/src/indicator-messages-service.h
)

set(
    SOURCES_GEN
    ${CMAKE_BINARY_DIR}/src/indicator-messages-service.c
)

set_source_files_properties(${HEADERS_GEN} ${SOURCES_GEN} PROPERTIES GENERATED TRUE)
add_library("indicator-messages-service" STATIC ${HEADERS} ${HEADERS_GEN} ${SOURCES} ${SOURCES_GEN})
target_include_directories("indicator-messages-service" PUBLIC ${PROJECT_DEPS_INCLUDE_DIRS})
target_compile_definitions("indicator-messages-service" PUBLIC G_LOG_DOMAIN="Ayatana-Indicator-Messages")
add_dependencies("indicator-messages-service" "ayatana-indicator-messages-service")

# test-gactionmuxer

add_executable("test-gactionmuxer" test-gactionmuxer.cpp)
target_link_options("test-gactionmuxer" PRIVATE -no-pie)
target_include_directories("test-gactionmuxer" PUBLIC ${PROJECT_DEPS_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/src")
target_link_libraries("test-gactionmuxer" "indicator-messages-service" ${PROJECT_DEPS_LIBRARIES} ${GTEST_LIBRARIES} ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES})
add_test("test-gactionmuxer" "test-gactionmuxer")
add_dependencies("test-gactionmuxer" "indicator-messages-service")
set(COVERAGE_TEST_TARGETS ${COVERAGE_TEST_TARGETS} "test-gactionmuxer" PARENT_SCOPE)

if (ENABLE_COVERAGE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_link_libraries("test-gactionmuxer" "--coverage")
    else()
        target_link_libraries("test-gactionmuxer" "-lgcov")
    endif()
endif()

# gschemas.compiled

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES gschemas.compiled)
set_source_files_properties(gschemas.compiled GENERATED)
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas OUTPUT_VARIABLE COMPILE_SCHEMA_EXECUTABLE OUTPUT_STRIP_TRAILING_WHITESPACE)
add_custom_command(OUTPUT gschemas.compiled COMMAND cp -f ${CMAKE_SOURCE_DIR}/data/org.ayatana.indicator.messages.gschema.xml ${CMAKE_CURRENT_BINARY_DIR} COMMAND ${COMPILE_SCHEMA_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target("gschemas-compiled" ALL DEPENDS gschemas.compiled)

# indicator-test

pkg_check_modules(DBUSTEST REQUIRED dbustest-1)
add_executable("indicator-test" indicator-test.cpp)
target_link_options("indicator-test" PRIVATE -no-pie)
target_include_directories("indicator-test" PUBLIC ${PROJECT_DEPS_INCLUDE_DIRS} ${DBUSTEST_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/libmessaging-menu")
target_link_libraries("indicator-test" "messaging-menu" ${PROJECT_DEPS_LIBRARIES} ${DBUSTEST_LIBRARIES} ${GTEST_LIBRARIES} ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES})
target_compile_definitions(
    "indicator-test"
    PUBLIC
    INDICATOR_MESSAGES_SERVICE_BINARY="${CMAKE_BINARY_DIR}/src/ayatana-indicator-messages-service"
    SCHEMA_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    XDG_DATA_DIRS="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test("indicator-test" "indicator-test")
add_dependencies("indicator-test" "messaging-menu" "gschemas-compiled")
set(COVERAGE_TEST_TARGETS ${COVERAGE_TEST_TARGETS} "indicator-test" PARENT_SCOPE)

# test-client.sh

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/test-client.sh" GENERATED)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test-client.sh" "export LD_LIBRARY_PATH=\"${CMAKE_BINARY_DIR}/libmessaging-menu\"; export GI_TYPELIB_PATH=\"${CMAKE_BINARY_DIR}/libmessaging-menu\"; export XDG_DATA_DIRS=\"${CMAKE_CURRENT_SOURCE_DIR}\"; python3 \"${CMAKE_CURRENT_SOURCE_DIR}/test-client.py\"")
add_test(NAME "test-client" COMMAND sh "${CMAKE_CURRENT_BINARY_DIR}/test-client.sh")
