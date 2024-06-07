

#include "acamera.h"
#include "imgsink.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
acamera::acamera(const dims_t& wh, const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):_mt(wh, n)
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
    _img_size = CFG["image"]["img_size"].to_dims();
    _tp = std::chrono::steady_clock::now();
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
    const event_t& e = _mt.proc_events(img, _name, _predicate);
    _mt.save_local(img, _name, _predicate, std::string());
    return e;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::save_events(const imglayout_t& img, const std::string& folder)
{
    _mt.save_local(img, _name, _predicate, folder);
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

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::set_fps(int fps)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  acamera::count_fps()
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::milliseconds interval = std::chrono::duration_cast<std::chrono::milliseconds>(now - _tp);
    if(interval>=std::chrono::milliseconds(1000))
    {
         TRACE() << "FPS = " << _real_fps << "\n";
        _real_fps=0;
        _tp = now;
    }
    else
    {
        ++_real_fps;
    }
}
