#include "acamera.h"
#include "imgsink.h"

acamera::acamera(const dims_t& wh, const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):_mt(wh, n["move"])
{
    _name     = name;
    _location = loc;
}

acamera::~acamera()
{
    if(_peer){
        delete _peer;
    }
}

void acamera::set_peer(imgsink* peer)
{
    _peer = peer;
}

const uint8_t* acamera::getm(int& w, int& h, int& sz)
{
    return _mt.getm(w,h,sz);
}

void acamera::proc_events(const imglayout_t& img)
{
    _mt.proc_events(img);
}

void  acamera::clean_events()
{
    _mt.clean_events();
}



