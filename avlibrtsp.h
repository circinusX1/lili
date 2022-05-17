
#ifndef AVRLIB_RTSP
#define AVRLIB_RTSP

#ifdef WITH_AVLIB_RTSP

#include <stdio.h>
#include <string>
#include "lilitypes.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
//-lavdevice -lavformat -lavcodec -lavutil ? no need, we link dynamically
}


#define FILE_MAX_CHECK  100000

typedef AVCodecContext* (*p_avcodec_alloc_context3)(const AVCodec *codec);
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
typedef enum AVCodecID (*p_av_guess_codec)(AVOutputFormat *fmt, const char *short_name,
                                           const char *filename, const char *mime_type,
                                           enum AVMediaType type);
typedef AVOutputFormat* (*p_av_guess_format)(const char *short_name,
                                                        const char *filename,
                                                        const char *mime_type);
typedef AVFormatContext* (*p_avformat_alloc_context)(void);
typedef int (*p_avformat_open_input)(AVFormatContext **ps, const char *url, AVInputFormat *fmt, AVDictionary **options);
typedef void (*p_av_dict_free)(AVDictionary **m);
typedef int (*p_avformat_find_stream_info)(AVFormatContext *ic, AVDictionary **options);
typedef int (*p_av_read_frame)(AVFormatContext *s, AVPacket *pkt);
typedef int (*p_avpicture_get_size)(enum AVPixelFormat pix_fmt, int width, int height);
typedef int (*p_avpicture_layout)(const AVPicture *src, enum AVPixelFormat pix_fmt,
                                  int width, int height,
                                  unsigned char *dest, int dest_size);
typedef int (*p_avformat_network_init)(void);
typedef int (*p_av_read_play)(AVFormatContext *s);
typedef int (*p_av_read_pause)(AVFormatContext *s);
typedef AVCodec* (*p_avcodec_find_decoder)(enum AVCodecID id);
typedef int (*p_avcodec_get_context_defaults3)(AVCodecContext *s, const AVCodec *codec);
typedef int (*p_avcodec_copy_context)(AVCodecContext *dest, const AVCodecContext *src);

typedef struct SwsContext* (*p_sws_getContext)(int srcW, int srcH, enum AVPixelFormat srcFormat,
                                               int dstW, int dstH, enum AVPixelFormat dstFormat,
                                               int flags, SwsFilter *srcFilter,
                                               SwsFilter *dstFilter, const double *param);
typedef int (*p_avcodec_decode_video2)(AVCodecContext *avctx, AVFrame *picture,
                                       int *got_picture_ptr,
                                       const AVPacket *avpkt);
typedef int (*p_sws_scale)(struct SwsContext *c, const uint8_t *const srcSlice[],
                           const int srcStride[], int srcSliceY, int srcSliceH,
                           uint8_t *const dst[], const int dstStride[]);

typedef int  (*p_avio_close)(AVIOContext *s);
typedef void (*p_avformat_free_context)(AVFormatContext *s);
typedef void* (*p_av_malloc)(size_t size) av_malloc_attrib av_alloc_size(1);
typedef void (*p_av_free)(void *ptr);
typedef void (*p_av_packet_unref)(AVPacket *pkt);

class avrlibrtsp
{
public:
    avrlibrtsp(const dims_t& d);
    ~avrlibrtsp();
    bool init(const std::string& url,
              const std::string& credentials, const dims_t& img_size);
    bool spin(Frame* frame);
    void destroy();

private:
    static int _netcam_rtsp_interrupt(void *ctx);
    void _decode(Frame& vframe, const AVPacket& packet);

private:
    void*                    _av_format_lib = nullptr;
    void*                    _av_codek_lib = nullptr;
    void*                    _swscale_lib = nullptr;
    void*                    _avutil_lib = nullptr;
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
    p_av_free                _pav_free;
    p_av_frame_get_buffer    _pav_frame_get_buffer;
    p_avcodec_receive_packet _pavcodec_receive_packet;
    p_av_guess_codec         _pav_guess_codec;
    p_av_guess_format        _pav_guess_format;
    p_avformat_alloc_context _pavformat_alloc_context;
    p_avformat_open_input    _pavformat_open_input;
    p_av_dict_free           _pav_dict_free;
    p_av_read_frame            _pav_read_frame;
    p_avformat_find_stream_info _pavformat_find_stream_info;
    p_avpicture_get_size        _pavpicture_get_size;
    p_avpicture_layout          _pavpicture_layout;
    p_avformat_network_init     _pavformat_network_init;
    p_av_read_play              _pav_read_play;
    p_av_read_pause             _pav_read_pause;
    p_avcodec_find_decoder      _pavcodec_find_decoder;
    p_avcodec_get_context_defaults3 _pavcodec_get_context_defaults3;
    p_avcodec_copy_context        _pavcodec_copy_context;
    p_sws_getContext              _psws_getContext;
    p_avcodec_decode_video2       _pavcodec_decode_video2;
    p_sws_scale                   _psws_scale;
    p_avio_close                  _pavio_close;
    p_avformat_free_context       _pavformat_free_context;
    p_av_malloc                   _pav_malloc;
    p_av_packet_unref             _pav_packet_unref;
    ///////////////////////////////////////////////////////////////////////////////////
    dims_t                          _dims;
    AVCodecContext*                 avc_ctx = nullptr;
    AVFormatContext*                avf_ctx = nullptr;
    int                             video_stream_index = 0;
    AVCodec*                        avc = nullptr;
    AVDictionary*                   option = nullptr;
    AVPixelFormat                   pix_fmt = AV_PIX_FMT_NONE;
    AVFrame*                        pFrame = nullptr;
    AVFrame*                        pFrame422 = nullptr;
    SwsContext*                     swsContext = nullptr;
};

#endif /// RTSP

#endif // MPEGER_H

