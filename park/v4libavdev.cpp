
#include <iostream>
#include <unistd.h>
#include "v4libavdev.h"
#include "liconfig.h"




camonlibav::camonlibav(const char *deviceName, FrameCallback cbParam):_sdevice(deviceName)
{

    cb = cbParam;
    av_register_all();
    avdevice_register_all();
    pInputFmt = av_find_input_format("video4linux2");
    if (pInputFmt == NULL) {
        av_log(0, AV_LOG_ERROR, "Cannot find input format\n");
        return ;
    }

    pFormatCtx = avformat_alloc_context();
    if (pFormatCtx == NULL)
    {
        av_log(0, AV_LOG_ERROR, "Cannot initialize context\n");
        return ;
    }

    std::string fps = std::to_string(GCFG->_glb.fps);
    char rez[32];
    ::sprintf(rez,"%dx%d",GCFG->_glb.w, GCFG->_glb.h);

    av_dict_set(&pOptions, "framerate", fps.c_str(), 0);
    av_dict_set(&pOptions, "input_format", "yuyv422", 0);
    //av_dict_set(&pOptions, "input_format", "yuyv422", 0);
    av_dict_set(&pOptions, "video_size",rez, 0);

    int opened = 0;
    size_t diez = _sdevice.find('*');
    if(diez != std::string::npos)
    {
        std::string sdev = _sdevice.substr(0,diez);
        for(int i=0; i < 8; i++)
        {
            std::string check = sdev + std::to_string(i);
            std::cout << "opening: " << check << "\n";
            if (::access(check.c_str(),0)!=0)
            {
                std::cout << "No Device:  "<< check << ", " <<  strerror(errno)  << "\n";
                continue;
            }
            opened = avformat_open_input(&pFormatCtx, check.c_str(), pInputFmt, &pOptions);

            if (-1 == opened)
            {
                std::cout << "Cannot open " << check << ", " <<  strerror(errno)  << "\n";
                continue;
            }
            break;
        }
    }
    else
    {
        if (::access(_sdevice.c_str(),0)!=0)
        {
            std::cout << "No Device: "<< _sdevice << " " <<  errno  << "\n";
            return;
        }
        opened = avformat_open_input(&pFormatCtx, _sdevice.c_str(), pInputFmt, &pOptions);
    }

    if (opened < 0)
    {
        av_log(0, AV_LOG_ERROR, "Cannot open input\n");
        return ;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        av_log(0, AV_LOG_ERROR, "Cannot find stream info\n");
        return ;
    }
    av_dump_format(pFormatCtx, 0, deviceName, 0);

    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1)
    {
        av_log(0, AV_LOG_ERROR, "Didn't find a video stream\n");
        return ;
    }

    pCodecParams1 = pFormatCtx->streams[videoStream]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParams1->codec_id);
    if (pCodec == NULL) {
        av_log(0, AV_LOG_ERROR, "Unsupported codec\n");
        return ;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL)
    {
        av_log(0, AV_LOG_ERROR, "Cannot allocate codec context\n");
        return ;
    }

    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0)
    {
        av_log(0, AV_LOG_ERROR, "Couldn't create codec context from parameters\n");
        return ;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        av_log(0, AV_LOG_ERROR, "Could not open codec\n");
        return ;
    }
    pRawFrame = av_frame_alloc();


    _codec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
    if(_codec)
    {
         _ctx = avcodec_alloc_context3(_codec);
         if(_ctx)
         {
             _ctx->bit_rate = 14756;
             _ctx->width = GCFG->_glb.w;
             _ctx->height = GCFG->_glb.h;
             _ctx->time_base.num = 1;
             _ctx->time_base.den = 25;
             _ctx->max_b_frames = 1;
             _ctx->pix_fmt = AV_PIX_FMT_YUYV422;
             _ctx->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL-1;
             if (avcodec_open2(_ctx, _codec, NULL) < 0)
             {
                 return;
             }
         }
    }
    _ok = _codec && _ctx;
}

camonlibav::~camonlibav()
{
    av_packet_unref(&rawPacket);
    av_frame_unref(pRawFrame);
    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&pCodecCtx);
    avcodec_free_context(&_ctx);
}

int   camonlibav::get_frame()
{
    // The function read_frame actually reads a packet not a frame :)
    // And when you whant to decode, you firstly send_packet and then receive_frame.
    int res = av_read_frame(pFormatCtx, &rawPacket);
    if (res < 0)
    {
        return -1;
    }

    if (rawPacket.stream_index == videoStream)
    {
        int frameFinished = 0;
        res = avcodec_send_packet(pCodecCtx, &rawPacket);
        // TODO : check error
send_again:
        res = avcodec_receive_frame(pCodecCtx, pRawFrame);
        if (res == AVERROR(EAGAIN))
        {
            goto send_again;
        }
        else if (res < 0)
        {
            return -1;
        }

        if (cb)
        {
            cb(this);
        }
    }
    av_packet_unref(&rawPacket);
    av_frame_unref(pRawFrame);
    return 0;
}
