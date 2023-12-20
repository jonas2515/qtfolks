project(qt-folks)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Standard install paths
include(GNUInstallDirs)

include(EnableCoverageReport)
#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -coverage" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -coverage" )
  ENABLE_COVERAGE_REPORT(TARGETS ${TELEPHONY_APP})
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Contacts REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FOLKS REQUIRED folks)
pkg_check_modules(FOLKS_TP REQUIRED folks-telepathy)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(TP_QT5 REQUIRED TelepathyQt5)

execute_process(
    COMMAND qmake -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_INSTALL_PLUGINS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

include_directories(${Qt5Core_INCLUDE_DIRS}
                    ${Qt5Contacts_INCLUDE_DIRS})

enable_testing()

add_subdirectory(qt-folks)
add_subdirectory(demo)
