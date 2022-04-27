#ifndef ACAMERA_H
#define ACAMERA_H

#include <string>
#include "cbconf.h"
#include "lilitypes.h"
#include "camevents.h"



class imgsink;
class acamera
{

public:
    acamera(const dims_t& wh, const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~acamera();

    virtual bool init(const dims_t&)=0;
    virtual size_t get_frame(const uint8_t** pb, EIMG_FMT& fmt)=0;
    virtual bool spin()=0;
    const std::string& name()const { return _name; }
    void set_flag(event_t flag){_flags = flag;}
    event_t get_flag()const{return _flags;}
    void set_peer(imgsink* peer);
    imgsink* peer(){return   _peer;}
    const uint8_t* getm(int& w, int& h, int& sz);
    void proc_events(const imglayout_t& img);
    void  clean_events();
protected:
    camevents       _mt;
    std::string     _name;
    dims_t          _img_size;
    std::string     _location;
    int             _fps;
    event_t         _flags = {0,0};
    imgsink*        _peer = nullptr;

};

#endif // ACAMERA_H
