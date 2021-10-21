# org.ayatana.indicator.session.gschema.xml

find_package(GSettings)
set(GSETTINGS_COMPILE ON)
add_schema("org.ayatana.indicator.messages.gschema.xml")

# ayatana-indicator-messages.service

pkg_check_modules(SYSTEMD systemd)

if (${SYSTEMD_FOUND})
    pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ayatana-indicator-messages.service.in" "${CMAKE_CURRENT_BINARY_DIR}/ayatana-indicator-messages.service" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ayatana-indicator-messages.service" DESTINATION "${SYSTEMD_USER_DIR}")
endif()

# ayatana-indicator-messages.desktop

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ayatana-indicator-messages.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/ayatana-indicator-messages.desktop" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ayatana-indicator-messages.desktop" DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart")

# org.ayatana.indicator.messages

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/org.ayatana.indicator.messages" DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/ayatana/indicators")

# icons/*

install(
    DIRECTORY
    icons/16x16
    icons/22x22
    icons/24x24
    icons/32x32
    icons/48x48
    icons/scalable
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor"
    FILES_MATCHING PATTERN "*.png" PATTERN "*.svg"
)
