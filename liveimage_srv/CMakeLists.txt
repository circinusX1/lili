cmake_minimum_required (VERSION 2.6)
PROJECT(liveimage_srv)


set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fexceptions -Wall -fpermissive -fstack-protector-all -g -O0")
add_definitions(-Dcimg_display=0 -Dcimg_use_jpeg  -Dcimg_display=0 -DDEBUG)
#add_definitions( -DDEBUG )
add_executable (liveimage_srv cliqueue.cpp
logger.cpp
oneproc.cpp
rawsock.cpp
sock.cpp
tcpjpgmpartcam.cpp
tcpsrv.cpp
tcpwebsock.cpp
fpipe.cpp
main.cpp
pool.cpp
sheller.cpp
tcpcamcli.cpp
tcplibav.cpp
tcpwebjpgcli.cpp )



TARGET_LINK_LIBRARIES( liveimage_srv pthread jpeg)
