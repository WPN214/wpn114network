if(APPLE)
    set(QZEROCONF_HEADERS
        qzeroconf/qzeroconf.h
        qzeroconf/bonjour_p.h)
    set(QZEROCONF_SOURCES
        qzeroconf/bonjour.cpp)
elseif(ANDROID)
    set(QZEROCONF_HEADERS
        qzeroconf/qzeroconf.h
        qzeroconf/avahi-qt/qt-watch.h
        qzeroconf/avahi-qt/qt-watch_p.h)
    set(QZEROCONF_SOURCES
        qzeroconf/avahicore.cpp
        qzeroconf/avahi-qt/qt-watch.cpp
        # avahi-common
        qzeroconf/avahi-common/address.c
        qzeroconf/avahi-common/alternative.c
        qzeroconf/avahi-common/domain.c
        qzeroconf/avahi-common/error.c
        qzeroconf/avahi-common/i18n.c
        qzeroconf/avahi-common/malloc.c
        qzeroconf/avahi-common/rlist.c
        qzeroconf/avahi-common/simple-watch.c
        qzeroconf/avahi-common/strlst.c
        qzeroconf/avahi-common/thread-watch.c
        qzeroconf/avahi-common/timeval.c
        qzeroconf/avahi-common/utf8.c
        # avahi-core
        qzeroconf/avahi-core/addr-util.c
        qzeroconf/avahi-core/announce.c
        qzeroconf/avahi-core/browse.c
        qzeroconf/avahi-core/browse-dns-server.c
        qzeroconf/avahi-core/browse-domain.c
        qzeroconf/avahi-core/browse-service.c
        qzeroconf/avahi-core/browse-service-type.c
        qzeroconf/avahi-core/cache.c
        qzeroconf/avahi-core/dns.c
        qzeroconf/avahi-core/domain-util.c
        qzeroconf/avahi-core/entry.c
        qzeroconf/avahi-core/fdutil.c
        qzeroconf/avahi-core/hashmap.c
        qzeroconf/avahi-core/iface.c
        qzeroconf/avahi-core/iface-linux.c
        qzeroconf/avahi-core/log.c
        qzeroconf/avahi-core/multicast-lookup.c
        qzeroconf/avahi-core/netlink.c
        qzeroconf/avahi-core/prioq.c
        qzeroconf/avahi-core/probe-sched.c
        qzeroconf/avahi-core/querier.c
        qzeroconf/avahi-core/query-sched.c
        qzeroconf/avahi-core/resolve-address.c
        qzeroconf/avahi-core/resolve-host-name.c
        qzeroconf/avahi-core/resolve-service.c
        qzeroconf/avahi-core/response-sched.c
        qzeroconf/avahi-core/rr.c
        qzeroconf/avahi-core/rrlist.c
        qzeroconf/avahi-core/server.c
        qzeroconf/avahi-core/socket.c
        qzeroconf/avahi-core/timeeventq.c
        qzeroconf/avahi-core/util.c
        qzeroconf/avahi-core/wide-area.c)
    elseif(UNIX AND NOT APPLE)
        set(QZEROCONF_HEADERS
            qzeroconf/qzeroconf.h
            qzeroconf/avahi-qt/qt-watch.h
            qzeroconf/avahi-qt/qt-watch_p.h)
        set(QZEROCONF_SOURCES
            qzeroconf/avahiclient.cpp
            qzeroconf/avahi-qt/qt-watch.cpp)
endif()

set(QZEROCONF_HEADERS ${QZEROCONF_HEADERS} qzeroconf/qzeroconfservice.h qzeroconf/qzeroconfglobal.h)
set(QZEROCONF_SOURCES ${QZEROCONF_SOURCES} qzeroconf/qzeroconfservice.cpp)

add_library(qzeroconf SHARED ${QZEROCONF_HEADERS} ${QZEROCONF_SOURCES})
target_link_libraries(qzeroconf Qt5::Core Qt5::Network)
target_include_directories(qzeroconf PUBLIC qzeroconf)
set_target_properties(qzeroconf PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

if(APPLE)
    target_link_libraries(qzeroconf -framework CoreServices)
elseif(ANDROID)
    target_compile_definitions(qzeroconf PRIVATE -DHAVE_STRLCPY -DGETTEXT_PACKAGE -DHAVE_NETLINK)
elseif(UNIX)
    target_link_libraries(qzeroconf Avahi::client Avahi::common)
endif()

add_library(mongoose SHARED mongoose/mongoose.c)
