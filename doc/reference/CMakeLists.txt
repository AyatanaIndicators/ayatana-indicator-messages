# messaging-menu-docs.xml

set(PACKAGE_VERSION ${PROJECT_VERSION})# Rewrite and drop once we've switched to CMake
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/messaging-menu-docs.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-docs.xml" @ONLY)

# messaging-menu-decl.txt
# messaging-menu-decl-list.txt
# messaging-menu-overrides.txt
# messaging-menu-sections.txt
# messaging-menu.types

find_program(GTKDOC_SCAN gtkdoc-scan)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-decl.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-decl-list.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-overrides.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-sections.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.types"
    DEPENDS "messaging-menu"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${GTKDOC_SCAN}
    --module=messaging-menu
    --source-dir="${CMAKE_SOURCE_DIR}/libmessaging-menu"
)

# scangobj.sh

list(JOIN PROJECT_DEPS_INCLUDE_DIRS " -I" GTKDOC_SCANGOBJ_INCLUDE_DIRS)
list(JOIN PROJECT_DEPS_LIBRARIES " -l" GTKDOC_SCANGOBJ_LIBRARIES)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/scangobj.sh" GENERATED)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/scangobj.sh" "LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH:${CMAKE_BINARY_DIR}/libmessaging-menu\" CFLAGS=\"-I${GTKDOC_SCANGOBJ_INCLUDE_DIRS}\" LDFLAGS=\"-l${GTKDOC_SCANGOBJ_LIBRARIES} -L${CMAKE_BINARY_DIR}/libmessaging-menu -lmessaging-menu\" gtkdoc-scangobj --module=messaging-menu")

# messaging-menu.actions"
# messaging-menu.args"
# messaging-menu.hierarchy"
# messaging-menu.interfaces"
# messaging-menu.prerequisites"
# messaging-menu.signals"

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.actions"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.args"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.hierarchy"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.interfaces"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.prerequisites"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.signals"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-decl.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-decl-list.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-overrides.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-sections.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.types"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/scangobj.sh"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND sh scangobj.sh
)

# messaging-menu-undeclared.txt
# messaging-menu-undocumented.txt
# messaging-menu-unused.txt
# sgml.stamp
# xml/annotation-glossary.xml
# xml/api-index-deprecated.xml
# xml/api-index-full.xml
# xml/messaging-menu-app.xml
# xml/messaging-menu-doc.bottom
# xml/messaging-menu-doc.top
# xml/messaging-menu-message.xml
# xml/object_index.sgml
# xml/tree_index.sgml

find_program(GTKDOC_MKDB gtkdoc-mkdb)
list(JOIN PROJECT_DEPS_INCLUDE_DIRS ";-I" GTKDOC_SCANGOBJ_INCLUDE_DIRS)
list(JOIN PROJECT_DEPS_LIBRARIES ";-l" GTKDOC_SCANGOBJ_LIBRARIES)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-undeclared.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-undocumented.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-unused.txt"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/annotation-glossary.xml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/api-index-deprecated.xml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/api-index-full.xml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-app.xml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-doc.bottom"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-doc.top"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/xml/messaging-menu-message.xml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/object_index.sgml"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml/tree_index.sgml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.actions"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.args"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.hierarchy"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.interfaces"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.prerequisites"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu.signals"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${GTKDOC_MKDB}
    --module=messaging-menu
    --output-format=xml
    --expand-content-files=""
    --main-sgml-file=messaging-menu-docs.xml
    --source-dir="${CMAKE_SOURCE_DIR}/libmessaging-menu"
    --xml-mode
)

# html

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html")

# html/annotation-glossary.html
# html/api-index-full.html
# html/ch01.html
# html/home.png
# html/index.html
# html/left.png
# html/left-insensitive.png
# html/messaging-menu.devhelp2
# html/MessagingMenuApp.html
# html/MessagingMenuMessage.html
# html/object-tree.html
# html/right.png
# html/right-insensitive.png
# html/style.css
# html/up.png
# html/up-insensitive.png

find_program(GTKDOC_MKHTML gtkdoc-mkhtml)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/annotation-glossary.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/api-index-full.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/ch01.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/home.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/left.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/left-insensitive.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/messaging-menu.devhelp2"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/MessagingMenuApp.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/MessagingMenuMessage.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/object-tree.html"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/right.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/right-insensitive.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/style.css"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/up.png"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/up-insensitive.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-undeclared.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-undocumented.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/messaging-menu-unused.txt"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/annotation-glossary.xml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/api-index-deprecated.xml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/api-index-full.xml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-app.xml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-doc.bottom"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/messaging-menu-doc.top"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/xml/messaging-menu-message.xml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/object_index.sgml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xml/tree_index.sgml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    COMMAND
    ${GTKDOC_MKHTML}
    --path=${CMAKE_CURRENT_BINARY_DIR}
    messaging-menu
    messaging-menu-docs.xml
)

# html/annotation-glossary.html
# html/api-index-deprecated.html
# html/api-index-full.html
# html/ch01.html
# html/index.html
# html/object-tree.html
# html/style.css

find_program(GTKDOC_FIXXREF gtkdoc-fixxref)

add_custom_command(
    OUTPUT "fixxref"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/annotation-glossary.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/api-index-full.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/ch01.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/home.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/left.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/left-insensitive.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/messaging-menu.devhelp2"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/MessagingMenuApp.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/MessagingMenuMessage.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/object-tree.html"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/right.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/right-insensitive.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/style.css"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/up.png"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/up-insensitive.png"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
    ${GTKDOC_FIXXREF}
    --module=messaging-menu
    --module-dir=html
    --html-dir="${CMAKE_INSTALL_FULL_DATADIR}/gtk-doc/html"
)

set_source_files_properties("fixxref" PROPERTIES SYMBOLIC TRUE)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html/" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/gtk-doc/html/messaging-menu")

add_custom_target("docs-references" ALL DEPENDS "fixxref")
