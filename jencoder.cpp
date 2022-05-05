
#include  "anytojpg.h"
#include  "fxxtojpg.h"
#include "jencoder.h"


jencoder::jencoder(int q, bool bw)
{
    _mpgenc = new mpeger(q, bw);
    _jpgenc = new jpeger(q, bw);
}

jencoder::~jencoder()
{
    delete _mpgenc;
    delete _jpgenc;
}

bool jencoder::init(const dims_t& d)
{
    _mpgenc->init(d);
    _jpgenc->init(d);
    return true;
}

int  jencoder::cam_to_jpg(imglayout_t& img)
{
    if(img._camf==e422){
        return _jpgenc->cam_to_jpg(img);
    }
    return _mpgenc->cam_to_jpg(img);
}

int  jencoder::cam_to_bw(imglayout_t& img)
{
    return _jpgenc->cam_to_bw(img);
}
