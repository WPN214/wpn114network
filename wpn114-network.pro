TARGET = WPN114-network
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

QMLDIR_FILES += $$PWD/qml/qmldir
QMLDIR_FILES += $$PWD/qml/network.qmltypes
OTHER_FILES = $$QMLDIR_FILES

qmlplugin.path = $$[QT_INSTALL_QML]/WPN114/Network
qmlplugin.files = $$QMLDIR_FILES
target.path = $$[QT_INSTALL_QML]/WPN114/Network

INSTALLS += target
INSTALLS += qmlplugin

DEFINES += QZEROCONF_STATIC
include (external/qzeroconf/qtzeroconf.pri)

HEADERS += $$PWD/source/http/http.hpp
HEADERS += $$PWD/source/osc/osc.hpp
HEADERS += $$PWD/source/oscquery/client.hpp
HEADERS += $$PWD/source/oscquery/device.hpp
HEADERS += $$PWD/source/oscquery/file.hpp
HEADERS += $$PWD/source/oscquery/folder.hpp
HEADERS += $$PWD/source/oscquery/node.hpp
HEADERS += $$PWD/source/oscquery/qserver.hpp
HEADERS += $$PWD/source/oscquery/tree.hpp
HEADERS += $$PWD/source/websocket/websocket.hpp

SOURCES += $$PWD/source/http/http.cpp
SOURCES += $$PWD/source/osc/osc.cpp
SOURCES += $$PWD/source/oscquery/client.cpp
SOURCES += $$PWD/source/oscquery/device.cpp
SOURCES += $$PWD/source/oscquery/file.cpp
SOURCES += $$PWD/source/oscquery/folder.cpp
SOURCES += $$PWD/source/oscquery/node.cpp
SOURCES += $$PWD/source/oscquery/qserver.cpp
SOURCES += $$PWD/source/oscquery/tree.cpp
SOURCES += $$PWD/source/websocket/websocket.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp
