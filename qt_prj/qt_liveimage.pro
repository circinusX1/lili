QT -= gui
QT -= core

CONFIG += c++11 console
CONFIG -= app_bundle
#QMAKE_LFLAGS += -no-pie


SOURCES += \
    ../acamera.cpp \
    ../cbconf.cpp \
    ../httpcam.cpp \
    ../imgsink.cpp \
    ../jpeger.cpp \
    ../localcam.cpp \
    ../mainn.cpp \
    ../motion.cpp \
    ../motion_track.cpp \
    ../mpeger.cpp \
    ../rtpudpcs.cpp \
    ../rtspcam.cpp \
    ../sock.cpp \
    ../sockserver.cpp \
    ../v4ldevice.cpp \
    ../webcast.cpp

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
    ../liveimage.konf

HEADERS += \
    ../acamera.h \
    ../cbconf.h \
    ../encoder.h \
    ../encrypter.h \
    ../httpcam.h \
    ../imgsink.h \
    ../jpeger.h \
    ../lilitypes.h \
    ../localcam.h \
    ../motion.h \
    ../motion_track.h \
    ../mpeger.h \
    ../os.h \
    ../encoder.h \
    ../rtpudpcs.h \
    ../rtspcam.h \
    ../sock.h \
    ../sockserver.h \
    ../strutils.h \
    ../v4ldevice.h \
    ../webcast.h


LIBS +=  -lpthread  -lv4l2 -ljpeg -ldl
# -lavdevice - -lavformat -lavcodec -lavutil
LIBS += -L$$usr/lib/x86_64-linux-gnu
