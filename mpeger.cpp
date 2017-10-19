///   EXPERIMENTAL

#include <dlfcn.h>
#include "mpeger.h"
#include "cbconf.h"

void* dls(void* v, const char* fn)
{
    std::cerr << fn << "\n";
    void* pr =  ::dlsym(v,fn);
    if(pr==nullptr)
    {
        char* error;
        if ((error = dlerror()) != NULL)  {
            std::cerr << error << "\n";
            return nullptr;
        }

    }
    return pr;
}


#define SO_SYM(foo) _p##foo = (p_##foo)::dls(_av_format_lib,static_cast<const char*>(#foo));


mpeger::mpeger(int q)
{
    (void)q;
}

bool mpeger::init(int w, int h)
{
    char* error;

    if(!CFG["I"]["lib_av"].exist())
    {
        return false;
    }
    _av_format_lib = ::dlopen(CFG["I"]["lib_av"].value().c_str(), RTLD_NOW);
    if(_av_format_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
    SO_SYM(avcodec_register_all);
    SO_SYM(avcodec_close);
    SO_SYM(avcodec_open2);
    SO_SYM(av_frame_alloc);
    SO_SYM(av_frame_free);
    SO_SYM(avcodec_alloc_context3);
    SO_SYM(avcodec_find_encoder);

    SO_SYM(avpicture_alloc);
    SO_SYM(avpicture_free);
    SO_SYM(avpicture_fill);

    SO_SYM(av_init_packet);
    SO_SYM(avcodec_encode_video2);
    SO_SYM(av_free_packet);

    _pavcodec_register_all();

    _codek = _pavcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!_codek) {
        fprintf(stderr, "codec not found\n");
        return false;
    }

    _pcodekCtx = _pavcodec_alloc_context3();
    if (!_pcodekCtx) {
        fprintf(stderr, "ERROR could not allocate memory for Format Context");
        return false;
    }

    _pcodekCtx->time_base.den = 1;
    _pcodekCtx->time_base.num = 1;
    _pcodekCtx->width   = w;
    _pcodekCtx->height  = h;
    _pcodekCtx->pix_fmt = AV_PIX_FMT_YUVJ420P; //AV_PIX_FMT_YUV420P;

    int error_val = _pavcodec_open2(_pcodekCtx,
                                    _codek, nullptr);
    if (error_val < 0) {
        fprintf(stderr, "could not open codec\n");
        return false;
    }
    _frame = _pav_frame_alloc();
    if (!_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return false;
    }
    _frame->format = _pcodekCtx->pix_fmt;
    _frame->width  = _pcodekCtx->width;
    _frame->height = _pcodekCtx->height;
    return true;
}

mpeger::~mpeger()
{
    delete[] _frame->data[0];
    _pavcodec_close(_pcodekCtx);
    dlclose(_av_format_lib);
}

uint32_t mpeger::convert420(const uint8_t* fmt420, int insz, int w,int h,
                            int jpg_quality, uint8_t** ppng)
{
    int  len = 0;
    (void)jpg_quality;
    AVPacket pkt;
    int got_picture = 0;
    (void)insz;

    if(_frame->data[0] == nullptr)
    {
        _frame->data[0] = new uint8_t[w*h*6];
    }

    _pav_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    _pavpicture_fill((AVPicture*)_frame,
                     fmt420, _pcodekCtx->pix_fmt, _pcodekCtx->width,
                     _pcodekCtx->height);

    len = _pavcodec_encode_video2(_pcodekCtx,
                                      &pkt, (AVFrame*)_frame,  &got_picture);
    if (len < 0)
    {
        fprintf(stderr, "Error while decoding\n");
        return false;
    }
    if (got_picture)
    {
        len = pkt.size;
        *ppng = pkt.data;
        _pav_free_packet(&pkt);
    }

    return len;
}


uint32_t mpeger::convertBW(const uint8_t* uint8buf, int insz, int w, int h,
                           int jpg_quality, uint8_t** pjpeg)

{
    (void)uint8buf;
    (void)insz;
    (void)h;
    (void)w;
    (void)jpg_quality;
    (void)pjpeg;
    return 0;
}
