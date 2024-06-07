QT -= gui
QT -= core
#QMAKE_LFLAGS += -no-pie

CONFIG += c++14 console
CONFIG -= app_bundle
DEFINES += cimg_use_jpeg
DEFINES += cimg_display=0
DEFINES += QTPRO WITH_AVLIB_RTSP
INCLUDEPATH += ../rtsp_class
INCLUDEPATH += ../
INCLUDEPATH += ./

# QMAKE_CFLAGS += -Wextra -Wfatal-errors -Werror=unknown-pragmas -Werror=unused-label -Wshadow -std=c++11 -pedantic -Dcimg_display=0 -Ofast -mtune=generic -lm


SOURCES += \
    ../acamera.cpp \
    ../avlibrtsp.cpp \
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
    ../CMakeLists.txt \
    ../liveimage.konf \
    ../liveimage.konf \
    ../liveimage.sample_konf

HEADERS += \
    ../CImg.h \
    ../acamera.h \
    ../anytojpg.h \
    ../avlibrtsp.h \
    ../camevents.h \
    ../cbconf.h \
    ../encrypter.h \
    ../fxxtojpg.h \
    ../imgsink.h \
    ../jencoder.cpp \
    ../jencoder.h \
    ../jencoder.h \
    ../jpeg_buffer.h \
    ../jpeghttpcam.h \
    ../lilitypes.h \
    ../localcam.h \
    ../motion.h \
    ../os.h \
    ../pipefile.h \
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


HEADERS += \
    ../CImg.h \
    ../acamera.h \
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
    ../rtpudpcs.h \
    ../rtspcam.h \
    ../sock.h \
    ../sockserver.h \
    ../strutils.h \
    ../v4ldevice.h \
    ../webcast.h \


LIBS +=  -lpthread  -lv4l2 -ljpeg -ldl
# -lavdevice - -lavformat -lavcodec -lavutil
LIBS += -L$$usr/lib/x86_64-linux-gnu
