# messaging-menu.pc

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")

# messaging-menu-app.h
# messaging-menu-message.h
# messaging-menu.h

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu-app.h" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/messaging-menu")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu-message.h" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/messaging-menu")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu.h" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/messaging-menu")

# libmessaging-menu.so

set(
    SOURCES
    messaging-menu-app.c
    messaging-menu-message.c
)

set(
    SOURCES_GEN
    ${CMAKE_BINARY_DIR}/src/indicator-messages-service.c
    ${CMAKE_BINARY_DIR}/src/indicator-messages-application.c
)

set_source_files_properties(${SOURCES_GEN} PROPERTIES GENERATED TRUE)
set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
add_library("messaging-menu" SHARED ${SOURCES} ${SOURCES_GEN})
set_target_properties("messaging-menu" PROPERTIES VERSION 0.0.0 SOVERSION 0)
target_include_directories("messaging-menu" PUBLIC ${PROJECT_DEPS_INCLUDE_DIRS} "${CMAKE_BINARY_DIR}/src")
target_link_options("messaging-menu" PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu.symbols")
target_link_libraries("messaging-menu" ${PROJECT_DEPS_LIBRARIES})
add_dependencies("messaging-menu" "ayatana-indicator-messages-service")
install(TARGETS "messaging-menu" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")

# MessagingMenu-1.0.gir

find_package(GObjectIntrospection REQUIRED)

set(
    HEADERS
    messaging-menu-app.h
    messaging-menu-message.h
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.gir"
    DEPENDS "messaging-menu"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND
    ${INTROSPECTION_SCANNER}
    ${HEADERS} ${SOURCES}
    --namespace=MessagingMenu
    --nsversion=1.0
    --quiet
    --warn-all
    --include=GObject-2.0
    --include=Gio-2.0
    --pkg-export=messaging-menu
    --library="messaging-menu"
    --c-include="messaging-menu.h"
    --library-path=${CMAKE_CURRENT_BINARY_DIR}
    --output "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.gir"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.gir" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/gir-1.0")

# MessagingMenu-1.0.typelib

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.typelib"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.gir"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${INTROSPECTION_COMPILER}
    --includedir=${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.gir
    -o "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.typelib"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.typelib" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/girepository-1.0")

# MessagingMenu-1.0.vapi

find_package(Vala REQUIRED)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.vapi"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.typelib"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${VAPI_GEN}
    --library=MessagingMenu-1.0
    --pkg gio-2.0
    MessagingMenu-1.0.gir
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.vapi" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/vala/vapi")

add_custom_target("libmessaging-menu" ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/MessagingMenu-1.0.vapi")
