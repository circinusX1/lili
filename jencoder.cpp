
#include  "fxxtojpg.h"
#include "jencoder.h"


jencoder::jencoder(int q, bool bw)
{
    _jpgenc = new jpeger(q, bw);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
jencoder::~jencoder()
{
    delete _jpgenc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool jencoder::init(const dims_t& d)
{
    _jpgenc->init(d);
    return true;
}

int  jencoder::cam_to_jpg(imglayout_t& img,const std::string& name)
{
    if(img._camf==e422){
        return _jpgenc->cam_to_jpg(img, name);
    }
    return 0;
}

int  jencoder::cam_to_bw_for_motion(imglayout_t& img)
{
    return _jpgenc->cam_to_bw_for_motion(img);
}
