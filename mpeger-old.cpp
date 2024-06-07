///   EXPERIMENTAL


#include "mpeger.h"
#include "cbconf.h"


mpeger::mpeger(int q, bool)
{
    (void)q;
}

mpeger::~mpeger()
{
    delete[] _frame->data[0];
    _pavcodec_close(_pcodekCtx);
    dlclose(_av_format_lib);
}

bool mpeger::init(const dims_t& imgsz)
{
    char* error;

    if(!CFG["image"]["lib_av"].exist())
    {
        return false;
    }
    const std::string& libloc = CFG["image"]["lib_av"].value();
    _av_format_lib = ::dlopen(libloc.c_str(), RTLD_NOW);
    if(_av_format_lib == nullptr)
    {
        error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    }
    SO_SYM(_av_format_lib, avcodec_register_all);
    SO_SYM(_av_format_lib, avcodec_close);
    SO_SYM(_av_format_lib, avcodec_open2);
    SO_SYM(_av_format_lib, av_frame_alloc);
    SO_SYM(_av_format_lib, av_frame_free);
    SO_SYM(_av_format_lib, avcodec_alloc_context3);
    SO_SYM(_av_format_lib, avcodec_find_encoder);

    SO_SYM(_av_format_lib, avpicture_alloc);
    SO_SYM(_av_format_lib, avpicture_free);
    SO_SYM(_av_format_lib, avpicture_fill);

    SO_SYM(_av_format_lib, av_init_packet);
    SO_SYM(_av_format_lib, avcodec_encode_video2);
    SO_SYM(_av_format_lib, av_free_packet);


    SO_SYM(_av_format_lib, av_packet_alloc);
    SO_SYM(_av_format_lib, av_packet_free);

    SO_SYM(_av_format_lib, avcodec_receive_frame);
    SO_SYM(_av_format_lib, avcodec_send_packet);


    _pavcodec_register_all();

    _codek = _pavcodec_find_encoder(AV_CODEC_ID_LJPEG);//AV_CODEC_ID_MJPEG);
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
    _pcodekCtx->width         = imgsz.x;
    _pcodekCtx->height        = imgsz.y;
    _pcodekCtx->pix_fmt       = AV_PIX_FMT_YUVJ420P; //AV_PIX_FMT_YUV420P;

    int error_val = _pavcodec_open2(_pcodekCtx, _codek, nullptr);
    if (error_val < 0) {
        fprintf(stderr, "could not open codec\n");
        return false;
    }
    /*
    _packet = _pav_packet_alloc();
    _packet->data = NULL;
    _packet->size = 0;
*/
    _frame = _pav_frame_alloc();
    if (!_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return false;
    }

    return true;
}


int mpeger::cam_to_jpg(const uint8_t* unkfmt, size_t len, int w,int h,
                       const uint8_t** ppng)
{
    AVPacket pkt;
    int      err, ilen = 0;

    for(int fmt = AV_PIX_FMT_YUV420P; fmt <   AV_PIX_FMT_NB; fmt++)
    {
        _pcodekCtx->pix_fmt = (AVPixelFormat)fmt;

        _frame->format = _pcodekCtx->pix_fmt;
        _frame->width  = _pcodekCtx->width;
        _frame->height = _pcodekCtx->height;
        _pavpicture_fill((AVPicture*)_frame,
                         unkfmt, _pcodekCtx->pix_fmt,
                         _pcodekCtx->width,
                         _pcodekCtx->height);

         // av_packet_alloc()
        AVPacket* packet = _pav_packet_alloc();
        _pav_init_packet(&packet);
        packet->data = (uint8_t*)unkfmt;
        packet->size = len;

        _pavcodec_send_packet(_pcodekCtx, _packet);

        AVFrame *frame = _pav_frame_alloc();
        int err = _pavcodec_receive_frame(_pcodekCtx, frame);
        _pav_packet_free(&_packet);

/*
        ilen = _pavcodec_encode_video2(_pcodekCtx,
                                      &pkt,
                                      (AVFrame*)_frame,
                                      &got_picture);

*/

        _pav_frame_free(&frame);

        if (err < 0)
        {
            fprintf(stderr, "Error while decoding\n");
            continue;
        }
        if (got_picture)
        {
            len = pkt.size;
            *ppng = pkt.data;
            char f[128];
            sprintf(f,"./_img_fmt%d.jpg",fmt);
            FILE* pf = fopen(f,  "wb");
            if(pf)
            {
                ::fwrite(*ppng,1,len, pf);
                ::fclose(pf);
            }
            _pav_free_packet(&pkt);
        }

    }


    return ilen;
}


int mpeger::cam_to_bw(const uint8_t* uint8buf, int w, int h, const uint8_t** pjpeg)

{
    (void)uint8buf;
    (void)h;
    (void)w;
    (void)pjpeg;
    return 0;
}
