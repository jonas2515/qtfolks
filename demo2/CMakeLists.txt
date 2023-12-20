
set(demo2_SRCS contactmodel.cpp demo.cpp)
set(demo2_HDRS contactmodel.h )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/qt-folks
    )

add_executable(demo2 ${demo2_SRCS} ${demo2_HDRS})

target_link_libraries(demo2
    ${TP_QT5_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Contacts_LIBRARIES}
    )
