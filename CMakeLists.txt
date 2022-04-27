cmake_minimum_required (VERSION 2.6)
PROJECT(liveimage)

###
### sudo apt-get install libpng-dev
### sudo apt-get install libv4l-dev
### sudo apt-get install libjpeg-dev
###

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
add_definitions(-Dcimg_display=0 -Dcimg_use_jpeg  -Dcimg_display=0)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fexceptions -Wall -fpermissive -fstack-protector-all -g -O0")
add_executable (liveimage
acamera.cpp
cbconf.cpp
httpcam.cpp
imgsink.cpp
jpeger.cpp
localcam.cpp
mainn.cpp
motion.cpp
camevents.cpp
mpeger.cpp
rtpudpcs.cpp
rtspcam.cpp
sock.cpp
sockserver.cpp
v4ldevice.cpp
webcast.cpp)
TARGET_LINK_LIBRARIES( liveimage pthread jpeg v4l2 dl)
