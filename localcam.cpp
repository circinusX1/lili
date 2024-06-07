#include "localcam.h"
#include "cbconf.h"

extern bool __alive;

///////////////////////////////////////////////////////////////////////////////////////////////////
localcam::localcam(const dims_t& wh,
                   const std::string& name,
                   const std::string& loc,
                   const Cbdler::Node& n):acamera(wh,name, loc, n)
{
    _device = loc;
    _img_size = CFG["image"]["img_size"].to_dims();
    _fps = n["fps"].to_int();
    if(_fps==0)
    {
        _interval=0;
    }
    else
    {
        _interval = 1000 / _fps;
        if(_interval < 30)_interval = 30;  // nomore than 30 fps

        else if(_interval > 1000)_interval = 1000;  // nless than 1fps
        _fetchtime = ::gtc();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
localcam::~localcam()
{
    if(_dev)
        _dev->close();
    delete _dev;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool  localcam::init(const dims_t& t)
{
    _img_size = t;
    _dev = new v4ldevice(_device.c_str(), t.x, t.y, _fps);
    if(_dev)
    {
        return _dev->open();
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void  localcam::set_fps(int fps)
{
    if(_dev)
        _dev->set_fps(fps);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t localcam::get_frame(imglayout_t& i)
{
    time_t  now = i._now;
    i._caml = 0;
    if(now - _fetchtime >= _interval)
    {
        bool fatal = false;
        int imgsz = 0;
        i._camp = _dev->read(_img_size.x, _img_size.y, imgsz, fatal);
        if(fatal || imgsz==0){
            return 0;
        }
        i._camf = e422;
        i._dims = _img_size;
        i._caml = imgsz;
        _fetchtime = now;
        this->count_fps();
    }
    this->count_fps();
    return (size_t)i._caml;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool localcam::spin()
{
    return true;
}

