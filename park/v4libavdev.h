#ifndef V4LIBAVDEV_H
#define V4LIBAVDEV_H

#include <stdio.h>
#include <string>
extern "C"{

    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavdevice/avdevice.h>
}


typedef struct PacketData {
    void *data;
    int error;
} PacketData;

class camonlibav;
typedef void (*FrameCallback) (camonlibav*);

class camonlibav
{
public:
    camonlibav(const char *deviceName, FrameCallback cbParam);
    ~camonlibav();

    int    get_frame();

public:
    bool        _ok = false;
    std::string _sdevice;

public:
    AVCodec* _codec = NULL;
    AVCodecContext *_ctx= NULL;

    AVInputFormat *pInputFmt = NULL;
    AVDictionary *pOptions = NULL;
    AVFormatContext *pFormatCtx = NULL;
    AVDeviceInfoList **ppDeviceInfoList = NULL;
    AVFrame *pRawFrame = NULL;
    AVPacket rawPacket;
    int videoStream = -1;
    AVCodecParameters *pCodecParams1 = NULL;
    AVCodec *pCodec = NULL;
    AVCodecContext *pCodecCtx = NULL;
    FrameCallback cb;
};

#endif // V4LIBAVDEV_H
