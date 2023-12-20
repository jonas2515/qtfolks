
set(demo_SRCS contactmodel.cpp demo.cpp demowindow.cpp)
set(demo_HDRS contactmodel.h demowindow.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${Qt5Qml_INCLUDE_DIRS}
    ${Qt5Quick_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/qt-folks
    )

add_executable(demo ${demo_SRCS} ${demo_HDRS})

target_link_libraries(demo
    ${TP_QT5_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Contacts_LIBRARIES}
    ${Qt5Qml_LIBRARIES}
    ${Qt5Quick_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    )
