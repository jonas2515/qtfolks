
set(qtfolks_SRCS managerengine.cpp engineid.cpp utils.cpp)
set(qtfolks_HDRS debug.h  glib-utils.h  managerengine.h engineid.h utils.h)

include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${FOLKS_INCLUDE_DIRS}
    ${FOLKS_TP_INCLUDE_DIRS}
    )

link_directories(${MESSAGING_MENU_LIBRARY_DIRS})

add_library(qtcontacts_folks MODULE ${qtfolks_SRCS} ${qtfolks_HDRS})

target_link_libraries(qtcontacts_folks
    ${TP_QT5_LIBRARIES}
    ${FOLKS_LIBRARIES}
    ${FOLKS_TP_LIBRARIES}
    ${GIO_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Contacts_LIBRARIES}
    )

install(TARGETS qtcontacts_folks LIBRARY DESTINATION ${QT_INSTALL_PLUGINS}/contacts)
