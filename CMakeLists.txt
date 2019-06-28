cmake_minimum_required(VERSION 3.1)
project(wpn114network-qml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_LINK_FLAGS)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

set(WPN114NETWORK_QML_SOURCES
    qml_plugin.cpp
    source/network.hpp
    source/network.cpp
    )

add_library(${PROJECT_NAME} SHARED ${WPN114NETWORK_QML_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml -lmongoose -lavahi-common -lavahi-client)

if(APPLE)
elseif(UNIX AND NOT APPLE)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/lib/qt/qml/WPN114/Network)
install(FILES qml/network.qmltypes qml/qmldir DESTINATION /usr/lib/qt/qml/WPN114/Network)
