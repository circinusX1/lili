#ifndef MPEGER_H
#define MPEGER_H

#include "encoder.h"
extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

typedef AVCodecContext* (*p_avcodec_alloc_context3)(void);
typedef void (*p_avformat_close_input)(AVFormatContext **s);
typedef void (*p_avcodec_register_all)(void);
typedef void (*p_av_register_input_format)(AVInputFormat *format);
typedef void (*p_av_register_output_format)(AVOutputFormat *format);
typedef AVStream* (*p_avformat_new_stream)(AVFormatContext *s, const AVCodec *c);
typedef AVCodec* (*p_avcodec_find_encoder)(enum AVCodecID id);
typedef int (*p_avcodec_open2)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
typedef int (*p_avcodec_close)(AVCodecContext *avctx);

typedef void (*p_av_init_packet)(AVPacket *pkt);
typedef AVFrame* (*p_av_frame_alloc)(void);
typedef void (*p_av_frame_free)(AVFrame **frame);
typedef int (*p_avpicture_alloc)(AVPicture *picture, enum AVPixelFormat pix_fmt, int width, int height);
typedef void (*p_avpicture_free)(AVPicture *picture);

typedef int (*p_avpicture_fill)(AVPicture *picture, const uint8_t *ptr,
                   enum AVPixelFormat pix_fmt, int width, int height);
typedef int (*p_avcodec_encode_video2)(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr);
typedef void (*p_av_free_packet)(AVPacket *pkt);


class mpeger : public encoder
{
public:
    mpeger(int q, bool bw);
    virtual ~mpeger();
    bool init(const dims_t& imgsz);
    int convert420(const uint8_t* fmt420, int insz, int w,int h, const uint8_t** ppng);
    int convertBW(const uint8_t* uint8buf, int insz, int w, int h,  const uint8_t** pjpeg);

private:


private:
    void*                    _av_format_lib;
    p_avcodec_alloc_context3 _pavcodec_alloc_context3;
    p_avformat_close_input   _pavformat_close_input;
    p_avcodec_register_all   _pavcodec_register_all;
    p_avformat_new_stream    _pavformat_new_stream;
    p_avcodec_find_encoder   _pavcodec_find_encoder;
    p_avcodec_open2          _pavcodec_open2;
    p_av_frame_alloc         _pav_frame_alloc;
    p_av_frame_free          _pav_frame_free;
    p_avcodec_close          _pavcodec_close;
    p_avpicture_alloc        _pavpicture_alloc;
    p_avpicture_free         _pavpicture_free;
    p_avpicture_fill         _pavpicture_fill;
    p_av_init_packet         _pav_init_packet;
    p_avcodec_encode_video2  _pavcodec_encode_video2;
    p_av_free_packet         _pav_free_packet;
    AVCodecContext*          _pcodekCtx;
    AVOutputFormat*          _pOutFormat;
    AVCodec *                _codek = nullptr;
    AVFrame *                _frame = nullptr;
    AVPicture*               _picture = nullptr;
    //lavdevice - -lavformat -lavcodec -lavutil
};

#endif // MPEGER_H
