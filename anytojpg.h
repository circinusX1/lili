#ifndef ANYTOJPG_H
#define ANYTOJPG_H

#include "cbconf.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}



class mpeger
{
public:
    mpeger(int q, bool bw);
    virtual ~mpeger();
    bool init(const dims_t& imgsz);
    int cam_to_jpg(imglayout_t& img);
    int cam_to_bw(imglayout_t& img);

private:


private:

    bool    _oneinit = false;
};

inline void savef(const uint8_t* p, size_t len, int seq)
{
    char fname[128];

    snprintf(fname, sizeof(fname), "./_img_fmt_%d.mpg", seq);
    FILE* pf = fopen(fname,  "wb");
    if(pf)
    {
        ::fwrite(p,1,len, pf);
        ::fclose(pf);
    }
}
#endif // ANYTOJPG_H
