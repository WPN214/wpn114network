TARGET = WPN114-network
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

localmod: DESTDIR = $$QML_MODULE_DESTDIR/WPN114/Network
else: DESTDIR = $$[QT_INSTALL_QML]/WPN114/Network

QMLDIR_FILES += $$PWD/qml/qmldir
QMLDIR_FILES += $$PWD/qml/network.qmltypes

for(FILE,QMLDIR_FILES) {
    QMAKE_POST_LINK += $$quote(cp $${FILE} $${DESTDIR}$$escape_expand(\n\t))
}

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
