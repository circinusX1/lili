#include "localcam.h"
#include "cbconf.h"

extern bool __alive;

localcam::localcam(const dims_t& wh,
                   const std::string& name,
                   const std::string& loc,
                   const Cbdler::Node& n):acamera(wh,name, loc, n)
{
    _device = loc;
    _img_size = CFG["image"]["img_size"].to_dims();
}

localcam::~localcam()
{
    if(_dev)
        _dev->close();
    delete _dev;
}

bool  localcam::init(const dims_t& t)
{
    _img_size = t;
    _dev = new v4ldevice(_device.c_str(), t.x, t.y);
    if(_dev)
    {
        return _dev->open();
    }
    return false;
}

size_t localcam::get_frame(imglayout_t& i)
{
    bool fatal = false;
    int imgsz = 0;
    i._camp = _dev->read(_img_size.x, _img_size.y, imgsz, fatal);
    if(fatal || imgsz==0){
        return 0;
    }
    i._camf = e422;
    i._dims.x = _img_size.x;
    i._dims.y = _img_size.y;
    i._caml = imgsz;
    return (size_t)imgsz;
}

bool localcam::spin()
{
    return true;
}

