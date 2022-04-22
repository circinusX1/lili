#ifndef ACAMERA_H
#define ACAMERA_H

#include <string>
#include "cbconf.h"
#include "lilitypes.h"

#define FLAG_RECORD     0x1
#define FLAG_WEBCAST    0x2
#define FLAG_TIMELAPSE  0x4
#define FLAG_SAVE       0x8
#define FLAG_FORCE_SAVE  0x10



class imgsink;
class acamera
{

public:
    acamera(const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~acamera();

    virtual bool init(const dims_t&)=0;
    virtual size_t get_frame(const uint8_t** pb, EIMG_FMT& fmt, event_t& event)=0;
    virtual bool spin(event_t& event)=0;
    const std::string& name()const { return _name; }
    void set_flag(event_t flag){_flags = flag;}
    event_t get_flag()const{return _flags;}
    void set_peer(imgsink* peer);
    imgsink* peer(){return   _peer;}
protected:
    std::string _name;
    dims_t      _img_size;
    std::string _location;
    int         _fps;
    event_t     _flags = {0,0};
    imgsink*    _peer = nullptr;
    std::string _motion;
};

#endif // ACAMERA_H
