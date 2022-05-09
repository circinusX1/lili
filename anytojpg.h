
#ifndef MPEGER_H
#define MPEGER_H

#ifdef WITH_RTSP

#include <string>
#include "lilitypes.h"
extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}


#define FILE_MAX_CHECK  100000


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
typedef AVPacket* (*p_av_packet_alloc)(void);
typedef void (*p_av_packet_free)(AVPacket **pkt);
typedef int (*p_avcodec_receive_frame)(AVCodecContext *avctx, AVFrame *frame);
typedef int (*p_avcodec_send_frame)(AVCodecContext *avctx, const AVFrame *frame);
typedef int (*p_avcodec_send_packet)(AVCodecContext *avctx, const AVPacket *avpkt);
typedef void (*p_av_freep)(void *ptr);
typedef int (*p_av_frame_get_buffer)(AVFrame *frame, int align);
typedef int (*p_avcodec_receive_packet)(AVCodecContext *avctx, AVPacket *avpkt);
typedef void (*p_avcodec_free_context)(AVCodecContext **avctx);
typedef enum AVCodecID (*p_av_guess_codec)(ff_const59 AVOutputFormat *fmt, const char *short_name,
                            const char *filename, const char *mime_type,
                            enum AVMediaType type);
typedef ff_const59 AVOutputFormat* (*p_av_guess_format)(const char *short_name,
                                const char *filename,
                                const char *mime_type);


class mpeger
{
public:
    mpeger(int q, bool bw);
    virtual ~mpeger();
    bool init(const dims_t& imgsz);
    int cam_to_jpg(imglayout_t& img, const std::string& name);
    int cam_to_bw_for_motion(const imglayout_t& img);

private:


private:
    void*                    _av_format_lib;
    void*                    _av_codek_lib;
    p_avcodec_alloc_context3 _pavcodec_alloc_context3;
    p_avcodec_free_context   _pavcodec_free_context;
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
    p_avcodec_receive_frame  _pavcodec_receive_frame;
    p_avcodec_send_frame     _pavcodec_send_frame = nullptr;
    p_av_packet_alloc        _pav_packet_alloc;
    p_av_packet_free         _pav_packet_free;
    p_avcodec_send_packet    _pavcodec_send_packet;
    p_av_freep               _pav_freep;
    p_av_frame_get_buffer    _pav_frame_get_buffer;
    p_avcodec_receive_packet _pavcodec_receive_packet;
    p_av_guess_codec         _pav_guess_codec;
    p_av_guess_format        _pav_guess_format;

    long                     _accum = 0;
    std::string              _facum;
    //lavdevice - -lavformat -lavcodec -lavutil
    const AVCodec *             codec= nullptr;
    AVCodecContext *            c= nullptr;
    AVPixelFormat             _last_good = AV_PIX_FMT_NONE;

};

#endif /// RTSP

#endif // MPEGER_H

