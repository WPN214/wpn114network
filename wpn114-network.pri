QT += network
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QZEROCONF_STATIC

include (external/qzeroconf/qtzeroconf.pri)

HEADERS += $$PWD/source/http/http.hpp
HEADERS += $$PWD/source/osc/osc.hpp
HEADERS += $$PWD/source/oscquery/client.hpp
HEADERS += $$PWD/source/oscquery/device.hpp
HEADERS += $$PWD/source/oscquery/file.hpp
HEADERS += $$PWD/source/oscquery/folder.hpp
HEADERS += $$PWD/source/oscquery/node.hpp
HEADERS += $$PWD/source/oscquery/server.hpp
HEADERS += $$PWD/source/oscquery/tree.hpp
HEADERS += $$PWD/source/websocket/websocket.hpp

SOURCES += $$PWD/source/http/http.cpp
SOURCES += $$PWD/source/osc/osc.cpp
SOURCES += $$PWD/source/oscquery/client.cpp
SOURCES += $$PWD/source/oscquery/device.cpp
SOURCES += $$PWD/source/oscquery/file.cpp
SOURCES += $$PWD/source/oscquery/folder.cpp
SOURCES += $$PWD/source/oscquery/node.cpp
SOURCES += $$PWD/source/oscquery/server.cpp
SOURCES += $$PWD/source/oscquery/tree.cpp
SOURCES += $$PWD/source/websocket/websocket.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp
