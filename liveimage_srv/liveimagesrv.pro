CONFIG   += c++14
CONFIG   += console
CONFIG   -= app_bundle
QT       -= gui

DEFINES += DEBUG

DEFINES += cimg_use_jpeg
DEFINES += cimg_display=0

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O0  -Wno-unused-parameter

QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG += -O0  -Wno-unused-parameter

SOURCES += main.cpp \
    cliqueue.cpp \
    fpipe.cpp \
    logger.cpp \
    oneproc.cpp \
    pool.cpp \
    rawsock.cpp \
    sheller.cpp \
    sock.cpp \
    tcpcamcli.cpp \
    tcpjpgmpartcam.cpp \
    tcplibav.cpp \
    tcpsrv.cpp \
    tcpwebjpgcli.cpp \
    tcpwebsock.cpp



LIBS += -L$$usr/lib/x86_64-linux-gnu

HEADERS += \
    CImg.h \
    cliqueue.h \
    config.h \
    encrypter.h \
    fpipe.h \
    logfile.h \
    logger.h \
    main.h \
    oneproc.h \
    os.h \
    pool.h \
    rawsock.h \
    sheller.h \
    sock.h \
    tcpcamcli.h \
    tcpjpgmpartcam.h \
    tcplibav.h \
    tcpsrv.h \
    tcpwebjpgcli.h \
    tcpwebsock.h

DISTFILES += \
    CMakeLists.txt \
    liveimage_srv.conf \
    liveimage_srv.conf \
    liveimage_srv.conf_example \
    mk_mov.sh \
    mk_video.sh

LIBS += -ljpeg
