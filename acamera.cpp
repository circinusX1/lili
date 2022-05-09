#include "acamera.h"
#include "imgsink.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
acamera::acamera(const dims_t& wh, const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):_mt(wh, n["move"])
{
    _name     = name;
    _location = loc;
    const Cbdler::Node& nev = n["on_event"];
    for(size_t ev=0;ev<nev.count();ev++){
        if(nev.value(ev)=="record"){
            _predicate |= FLAG_RECORD;  // @SERVER
        }
        else if(nev.value(ev)=="webcast"){
            _predicate |= FLAG_WEBCAST;
        }
        else if(nev.value(ev)=="save"){
            _predicate |= FLAG_SAVE;
        }
        else if(nev.value(ev)=="force"){
            _predicate |= FLAG_FORCE_SAVE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
acamera::~acamera()
{
    if(_peer){
        delete _peer;
    }
    delete _frames;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void acamera::init_frames()
{
    _frames = new DuoFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void acamera::set_peer(imgsink* peer)
{
    _peer = peer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const uint8_t* acamera::getm(int& w, int& h, int& sz)
{
    return _mt.getm(w,h,sz);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const event_t&  acamera::proc_events(const imglayout_t& img)
{
    return _mt.proc_events(img, _name, _predicate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::clean_events()
{
    _mt.clean_events();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::set_motion(const dims_t& mohilo, int pixnoise, int pixdiv, int imgscale)
{
    _mt.set(mohilo, pixnoise, pixdiv, imgscale);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::get_motion(dims_t& mohilo, int& pixnoise, int& pixdiv, int& imgscale)
{
    _mt.get(mohilo, pixnoise, pixdiv, imgscale);
}
