cmake_minimum_required(VERSION 3.1)
project(wpn114network LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# REQUIREMENTS ------------------------------------------------------------------------------------

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml Network)

if(UNIX AND NOT ANDROID AND NOT APPLE)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
    find_package(Avahi REQUIRED COMPONENTS common client)
endif()

# SOURCES -----------------------------------------------------------------------------------------

set(WPN114_NETWORK_SOURCE_DIR source)
set(WPN114_NETWORK_QML_DIR qml)

set(WPN114_NETWORK_SOURCES
    qml_plugin.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/network.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/network.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/osc.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/osc.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/server.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/server.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/client.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/client.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/node.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/node.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/file.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/file.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/directory.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/directory.cpp
    ${WPN114_NETWORK_SOURCE_DIR}/tree.hpp
    ${WPN114_NETWORK_SOURCE_DIR}/tree.cpp
    ${WPN114_NETWORK_QML_DIR}/qmldir
    ${WPN114_NETWORK_QML_DIR}/network.qmltypes)

# PROJECT -----------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} SHARED ${WPN114_NETWORK_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

# DEPENDENCIES ------------------------------------------------------------------------------------

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
add_subdirectory(dependencies)
add_subdirectory(examples)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml mongoose qzeroconf)

# INSTALLING --------------------------------------------------------------------------------------

if(EXISTS ${CMAKE_PREFIX_PATH} AND
   ${CMAKE_PREFIX_PATH} STREQUAL "/usr")
    set(QT5_QML_INSTALL_PATH /usr/lib/qt/qml)
else()
    set(QT5_QML_INSTALL_PATH ${CMAKE_PREFIX_PATH}/qml)
endif()

set(WPN114_NETWORK_INSTALL_PATH ${QT5_QML_INSTALL_PATH}/WPN114/Network)
message("[WPN114] Library will be installed in ${WPN114_NETWORK_INSTALL_PATH}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114_NETWORK_INSTALL_PATH})
install(FILES qml/network.qmltypes qml/qmldir DESTINATION ${WPN114_NETWORK_INSTALL_PATH})
