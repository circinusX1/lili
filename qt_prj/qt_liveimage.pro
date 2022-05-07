QT -= gui
QT -= core
#QMAKE_LFLAGS += -no-pie

CONFIG += c++14 console
CONFIG -= app_bundle
CONFIG += -O0
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE += -O0
QMAKE_CXXFLAGS_DEBUG -= -O1
QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG -= -O3
QMAKE_CXXFLAGS_DEBUG += -O0

DEFINES += cimg_use_jpeg
DEFINES += cimg_display=0
DEFINES += QTPRO WITH_RTSP
INCLUDEPATH += ../rtsp_class
INCLUDEPATH += ../
INCLUDEPATH += ./

# QMAKE_CFLAGS += -Wextra -Wfatal-errors -Werror=unknown-pragmas -Werror=unused-label -Wshadow -std=c++11 -pedantic -Dcimg_display=0 -Ofast -mtune=generic -lm


SOURCES += \
    ../acamera.cpp \
    ../anytojpg.cpp \
    ../camevents.cpp \
    ../cbconf.cpp \
    ../fxxtojpg.cpp \
    ../imgsink.cpp \
    ../jpeghttpcam.cpp \
    ../localcam.cpp \
    ../jencoder.cpp \
    ../mainn.cpp \
    ../motion.cpp \
    ../pipefile.cpp \
    ../rtspcam.cpp \
    ../rtspproto.cpp \
    ../sock.cpp \
    ../sockserver.cpp \
    ../v4ldevice.cpp \
    ../webcast.cpp


HEADERS += \
    ../CImg.h \
    ../acamera.h \
    ../anytojpg.h \
    ../camevents.h \
    ../cbconf.h \
    ../encrypter.h \
    ../fxxtojpg.h \
    ../imgsink.h \
    ../jencoder.cpp \
    ../jencoder.h \
    ../jpeghttpcam.h \
    ../lilitypes.h \
    ../localcam.h \
    ../motion.h \
    ../os.h \
    ../rtspcam.h \
    ../rtspproto.h \
    ../sock.h \
    ../sockserver.h \
    ../strutils.h \
    ../v4ldevice.h \
    ../webcast.h \



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../../../../var/www/html/camera.php \
    ../../../../var/www/html/camera_old.php \
    ../../../../var/www/html/server.php \
    ../../../../var/www/html/stream.php \
    ../../../../var/www/html/stream2.php \
    ../../../../var/www/html/upload.php \
    ../CMakeLists.txt \
    ../liveimage.konf \
    ../liveimage.sample_konf



LIBS +=  -lpthread  -lv4l2 -ljpeg -ldl
# -lavdevice - -lavformat -lavcodec -lavutil
LIBS += -L$$usr/lib/x86_64-linux-gnu




LIBS +=  -lpthread  -lv4l2 -ljpeg -ldl
# -lavdevice - -lavformat -lavcodec -lavutil
LIBS += -L$$usr/lib/x86_64-linux-gnu
