
#include  "anytojpg.h"
#include  "fxxtojpg.h"
#include "jencoder.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
jencoder::jencoder(int q, bool bw)
{
#ifdef WITH_RTSP
    _mpgenc = new mpeger(q, bw);
#endif
    _jpgenc = new jpeger(q, bw);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
jencoder::~jencoder()
{
#ifdef WITH_RTSP
    delete _mpgenc;
#endif
    delete _jpgenc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool jencoder::init(const dims_t& d)
{
#ifdef WITH_RTSP
    _mpgenc->init(d);
#endif
    _jpgenc->init(d);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int  jencoder::cam_to_jpg(imglayout_t& img, const std::string& name)
{
    if(img._camf==e422){
        return _jpgenc->cam_to_jpg(img, name);
    }
#ifdef WITH_RTSP
    return _mpgenc->cam_to_jpg(img, name);
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int  jencoder::cam_to_bw_for_motion(imglayout_t& img)
{
    return _jpgenc->cam_to_bw_for_motion(img);
}
