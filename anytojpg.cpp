///   EXPERIMENTAL

#include <unistd.h>
#include "anytojpg.h"
#include "cbconf.h"


mpeger::mpeger(int q, bool)
{
    (void)q;
}

mpeger::~mpeger()
{
    _pavcodec_free_context(&c);
    dlclose(_av_codek_lib);
}

bool mpeger::init(const dims_t& imgsz)
{
    char* error;

    if(!CFG["image"]["lib_av"].exist())
    {
        return false;
    }
    const std::string& libloc = CFG["image"]["lib_av"].value();

    std::string cdeklib = libloc + "codec.so";
    std::string fmtlib = libloc + "format.so";

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
    SO_SYM(_av_format_lib, av_guess_codec);
    SO_SYM(_av_format_lib, av_guess_format);
    SO_SYM(_av_format_lib, avcodec_send_frame);

    _pavcodec_register_all();
    codec = _pavcodec_find_encoder(AV_CODEC_ID_MJPEG);
    c = _pavcodec_alloc_context3();

    return true;
}

int mpeger::cam_to_jpg(imglayout_t& img, const std::string& name)
{
    std::string facum = "/tmp/movie.mov";
/*
    std::string facum = "./mov-";
    facum += name;
    facum += ".mov";
    if(img._acum<FILE_MAX_CHECK)
    {
        TRACE()<< "tmp=" << facum <<"\n";
        FILE *pff;
        if(::access(facum.c_str(),0)==0){
            pff = ::fopen(_facum.c_str(),"ab");
        }else{
            pff = ::fopen(_facum.c_str(),"wb");
        }

        if(pff)
        {
            ::fwrite(img._camp, 1, img._caml, pff);
            ::fclose(pff);
            img._acum+=img._caml;
        }
        return 0;
    }
*/

    AVOutputFormat* pf = _pav_guess_format(nullptr, facum.c_str(),nullptr);
    AVFrame *picture  = _pav_frame_alloc();
    AVPacket *pkt = _pav_packet_alloc();
    AVFormatContext * pfc = NULL;


    c->width = img._dims.x;
    c->height = img._dims.y;
    c->time_base.num = 1;
    c->time_base.den = 1;c->pix_fmt = AV_PIX_FMT_YUV420P; //img._camf;

    for(int i=AV_PIX_FMT_YUV420P;i<AV_PIX_FMT_GRAY16BE;i++)
    {

        if(0==_pavcodec_open2(c, codec, NULL))
            break;
        _pavcodec_close(c);
        c->pix_fmt = (AVPixelFormat)i;
        c->width = img._dims.x;
        c->height = img._dims.y;
        c->time_base.num = 1;
        c->time_base.den = 1;
    }

    //AVCodecID codekid = _pav_guess_codec(pfc->oformat, NULL, facum.c_str(),NULL,AVMEDIA_TYPE_VIDEO);

    picture->format = c->pix_fmt;
    picture->width  = c->width;
    picture->height = c->height;

    _pav_frame_get_buffer(picture, 32);
    ::memcpy(picture->data[0],img._camp,img._caml);

    int ret = _pavcodec_send_frame(c, picture);
    while (ret >= 0) {
        ret = _pavcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "error during encoding\n");
            break;
        }
        printf("encoded frame %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        if(img._jpgl==0)
        {
            img._jpgp = pkt->data;
            img._jpgl = pkt->size;
            img._jpgf = eFJPG;
        }
    }
    _pav_frame_free(&picture);
    _pav_packet_free(&pkt);
    return img._jpgl;
}


int mpeger::cam_to_bw(const imglayout_t&)
{
    return 0;
}
