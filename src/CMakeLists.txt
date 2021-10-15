include(GdbusCodegen)
set(SOURCES_GEN)

# indicator-messages-service.h
# indicator-messages-service.c

add_gdbus_codegen_with_namespace(
    SOURCES_GEN indicator-messages-service
    org.ayatana.indicator.messages
    IndicatorMessages
    ${CMAKE_SOURCE_DIR}/common/org.ayatana.indicator.messages.service.xml
)

# indicator-messages-application.h
# indicator-messages-application.c

add_gdbus_codegen_with_namespace(
    SOURCES_GEN indicator-messages-application
    org.ayatana.indicator.messages
    IndicatorMessages
    ${CMAKE_SOURCE_DIR}/common/org.ayatana.indicator.messages.application.xml
)

# ayatana-indicator-messages-service

set(
    SOURCES
    gactionmuxer.c
    gsettingsstrv.c
    im-accounts-service.c
    im-application-list.c
    im-desktop-menu.c
    im-menu.c
    im-phone-menu.c
    indicator-desktop-shortcuts.c
    messages-service.c
)

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
add_executable("ayatana-indicator-messages-service" ${SOURCES} ${SOURCES_GEN})
target_compile_definitions(
    "ayatana-indicator-messages-service" PUBLIC
    G_LOG_DOMAIN="Ayatana-Indicator-Messages"
    GETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
    LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)
target_include_directories("ayatana-indicator-messages-service" PUBLIC ${PROJECT_DEPS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries("ayatana-indicator-messages-service" ${PROJECT_DEPS_LIBRARIES})
install(TARGETS "ayatana-indicator-messages-service" RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ayatana-indicator-messages")
