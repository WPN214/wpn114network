cmake_minimum_required(VERSION 3.1)
project(wpn114network-qml LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml Network)

set(WPN114NETWORK_QML_SOURCES
    qml_plugin.cpp
    source/network.hpp
    source/network.cpp
    source/tree.hpp
    source/tree.cpp)

add_subdirectory(examples)
add_subdirectory(dependencies)

add_library(${PROJECT_NAME} SHARED ${WPN114NETWORK_QML_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC dependencies/avahi)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml mongoose)

if(APPLE)
elseif(UNIX AND NOT APPLE)
endif()

install(TARGETS ${PROJECT_NAME} mongoose
        DESTINATION /usr/lib/qt/qml/WPN114/Network)

install(FILES qml/network.qmltypes qml/qmldir
        DESTINATION /usr/lib/qt/qml/WPN114/Network)
