///   EXPERIMENTAL
#ifdef WITH_AVLIB_RTSP

#include <unistd.h>
#include "avlibrtsp.h"
#include "cbconf.h"


////////////////////////////////////////////////////////////////////////////////////////
#define RNDTO2(X)  (X & 0xFFFFFFFE )
#define RNDTO32(X) ( ( (X) % 32 ) ? ( ( (X) + 32 ) & 0xFFFFFFE0 ) : (X) )


////////////////////////////////////////////////////////////////////////////////////////
avrlibrtsp::avrlibrtsp(const dims_t& d):_dims(d)
{
}

////////////////////////////////////////////////////////////////////////////////////////
avrlibrtsp::~avrlibrtsp()
{
    destroy();
}

////////////////////////////////////////////////////////////////////////////////////////
void avrlibrtsp::destroy()
{
    if(pFrame422)
        _pav_free(pFrame422);

    if(pFrame)
        _pav_free(pFrame);

    if(avc_ctx)
        _pavcodec_close(avc_ctx);

    if(avf_ctx)
        _pavformat_close_input(&avf_ctx);

    avf_ctx=nullptr;
    avc_ctx=nullptr;
    pFrame = nullptr;
    pFrame422 = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////
int avrlibrtsp::_netcam_rtsp_interrupt(void *ctx)
{
    avrlibrtsp* pthis = static_cast<avrlibrtsp*>(ctx);
    (void)pthis;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
bool avrlibrtsp::init(const std::string& url,
                      const std::string& credentials,
                      const dims_t&)
{
    char* error;
    char            link[256];

    if(!CFG["image"]["lib_av"].exist())
    {
        return false;
    }
    const std::string& libloc = CFG["image"]["lib_av"].value();

    std::string cdeklib = libloc + "codec.so";
    std::string fmtlib = libloc + "format.so";
    std::string scalelib = "libswscale.so";
    std::string utillib = "libavutil.so";

    _av_codek_lib = ::dlopen(cdeklib.c_str(), RTLD_NOW);
    if(_av_codek_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
    _av_format_lib = ::dlopen(fmtlib.c_str(), RTLD_NOW);
    if(_av_format_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
#if 1
    _swscale_lib = ::dlopen(scalelib.c_str(), RTLD_NOW);
    if(_swscale_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
    _avutil_lib = ::dlopen(utillib.c_str(), RTLD_NOW);
    if(_avutil_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
#endif
    SO_SYM(_av_codek_lib, av_malloc);
    SO_SYM(_av_codek_lib, av_free);
    SO_SYM(_av_codek_lib, avcodec_register_all);
    SO_SYM(_av_codek_lib, avcodec_close);
    SO_SYM(_av_codek_lib, avcodec_open2);
    SO_SYM(_av_codek_lib, av_frame_alloc);
    SO_SYM(_av_codek_lib, av_frame_free);
    SO_SYM(_av_codek_lib, avcodec_alloc_context3);
    SO_SYM(_av_codek_lib, avcodec_find_encoder);
    SO_SYM(_av_codek_lib, avpicture_alloc);
    SO_SYM(_av_codek_lib, avpicture_free);
    SO_SYM(_av_codek_lib, avpicture_fill);
    SO_SYM(_av_codek_lib, av_init_packet);
    SO_SYM(_av_codek_lib, avcodec_encode_video2);
    SO_SYM(_av_codek_lib, av_free_packet);
    SO_SYM(_av_codek_lib, av_packet_alloc);
    SO_SYM(_av_codek_lib, av_packet_free);
    SO_SYM(_av_codek_lib, avcodec_receive_frame);
    SO_SYM(_av_codek_lib, avcodec_send_packet);
    SO_SYM(_av_codek_lib, av_freep);
    SO_SYM(_av_codek_lib, av_frame_get_buffer);
    SO_SYM(_av_codek_lib, avcodec_receive_packet);
    SO_SYM(_av_codek_lib, avcodec_free_context);
    SO_SYM(_av_codek_lib, avpicture_get_size);
    SO_SYM(_av_codek_lib, avpicture_layout);
    SO_SYM(_av_codek_lib, avcodec_find_decoder);
    SO_SYM(_av_codek_lib, avcodec_get_context_defaults3);
    SO_SYM(_av_codek_lib, avcodec_copy_context);
    SO_SYM(_av_codek_lib, avcodec_decode_video2);
    SO_SYM(_av_codek_lib, av_packet_unref);
    SO_SYM(_av_format_lib, av_malloc);
    SO_SYM(_av_format_lib, av_guess_codec);
    SO_SYM(_av_format_lib, av_guess_format);
    SO_SYM(_av_format_lib, avcodec_send_frame);
    SO_SYM(_av_format_lib, avformat_alloc_context);
    SO_SYM(_av_format_lib, avformat_open_input);
    SO_SYM(_av_format_lib, av_dict_free);
    SO_SYM(_av_format_lib, avformat_find_stream_info);
    SO_SYM(_av_format_lib, av_read_frame);
    SO_SYM(_av_format_lib, avio_close);
    SO_SYM(_av_format_lib, avformat_network_init);
    SO_SYM(_av_format_lib, av_read_play);
    SO_SYM(_av_format_lib, av_read_pause);
    SO_SYM(_av_format_lib,avformat_free_context);
#if 1
    SO_SYM(_swscale_lib, sws_getContext);
    SO_SYM(_swscale_lib, sws_scale);
#endif

    if(!credentials.empty())
    {
        char            scheme[8];
        int             port = 80;
        char            host[32];
        char            path[64];

        ::parse_url(url.c_str(), scheme,
                    sizeof(scheme), host, sizeof(host),
                    &port, path, sizeof(path));

        ::snprintf(link, sizeof(link), "%s://%s@%s:%d%s", scheme,
                   credentials.c_str(),
                   host,
                   port, path);
    }
    else {
        ::strncpy(link, url.c_str(), sizeof(link));
    }

    _pavcodec_register_all();
    _pavformat_network_init();
    avf_ctx = _pavformat_alloc_context();

    // AVDictionary* option = nullptr;
    // _pav_dict_set(&option, "rtsp_transport", "tcp", 0);

    if (_pavformat_open_input(&avf_ctx, link, nullptr, &option) != 0)
    {
        printf("avformat_open_input error\n");
        return false;
    }

    if (_pavformat_find_stream_info(avf_ctx,  nullptr) <0 )
    {
        destroy();
        printf("avformat_find_stream_info error\n");
        return false;
    }

    for (unsigned int i = 0; i < avf_ctx->nb_streams; i++)
    {
#if DEPRECATED
        if (avf_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            break;
        }
#endif // 0
        if(_pavcodec_find_encoder(avf_ctx->streams[i]->codec->codec_id)->type ==AVMEDIA_TYPE_VIDEO)
        {
            break;
        }
    }

    if (video_stream_index == -1)
    {
        destroy();
        printf("avformat_find_stream_info error\n");
        return false;
    }
    avc_ctx = avf_ctx->streams[video_stream_index]->codec;
    if(nullptr == avc_ctx)
    {
        printf("null codek found\n");
        destroy();
        return false;
    }
    avc = _pavcodec_find_decoder(avc_ctx->codec_id);
    if (avc == nullptr)
    {
        printf("avcodec_find_decoder error\n");
        destroy();
        return false;
    }

    if(_pavcodec_open2(avc_ctx, avc, nullptr) < 0)
    {
        destroy();
        printf("avcodec_open2 error\n");
        return false;
    }
    pix_fmt = avc_ctx->pix_fmt;
    pFrame = _pav_frame_alloc();
    pFrame422 = _pav_frame_alloc();

    pFrame422->format = AV_PIX_FMT_YUV420P;
    pFrame422->width  = _dims.x; /* must match sizes as on sws_getContext() */
    pFrame422->height = _dims.y; /* must match sizes as on sws_getContext() */

    if(pix_fmt != AV_PIX_FMT_YUV420P)
    {
        destroy();
        printf("pix format not supported\n");
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////
bool avrlibrtsp::spin(Frame* vframe)
{
    AVPacket packet;
    _pav_init_packet(&packet);

    if (_pav_read_frame(avf_ctx, &packet) >= 0)
    {
        if( (packet.stream_index == video_stream_index) )
        {
            if(vframe && packet.size)
            {
                vframe->_wh.x = avc_ctx->width;
                vframe->_wh.y = avc_ctx->height;
                _decode(*vframe, packet);
            }
            else { /* discard */}
        }
        _pav_packet_unref(&packet);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////
void avrlibrtsp::_decode(Frame& vframe, const AVPacket& packet)
{
    if(nullptr == swsContext)
    {
        swsContext = _psws_getContext(avc_ctx->width, avc_ctx->height, avc_ctx->pix_fmt,
                                      _dims.x, _dims.y, AV_PIX_FMT_YUV420P,
                                      SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }
    if(swsContext)
    {
        int ret = _pavcodec_send_packet(avc_ctx, &packet);
        if(ret>=0)
        {
            ret = _pavcodec_receive_frame(avc_ctx, pFrame);
            if(ret>=0)
            {
                ret = _pav_frame_get_buffer(pFrame422, 32);
                if(0==ret)
                {
                    ret = _psws_scale(swsContext,
                                      pFrame->data,
                                      pFrame->linesize,
                                      0,
                                      pFrame->height,
                                      pFrame422->data,
                                      pFrame422->linesize);
                    if(ret>=0)
                    {
                        vframe.reset();
                        vframe.copy(pFrame422->data[0],0,pFrame422->buf[0]->size);
                        vframe.ready();
                    }
                }
            }
        }
    }
}

#endif
