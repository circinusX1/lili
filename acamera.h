#ifndef ACAMERA_H
#define ACAMERA_H

#include <string>
#include "cbconf.h"
#include "lilitypes.h"
#include "camevents.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class DuoFrame{
public:
    DuoFrame(){
        _duo[0].reset();
        _duo[1].reset();
    }

    Frame&  write(){
        return _duo[_locked];
    }

    Frame&  read(){
        if(_duo[_locked].is_ready())
            return _duo[_locked];
        return _duo[!_locked];
    }

    void flip(){
        _locked = !_locked;
        _duo[_locked].reset();
    }

private:
    Frame   _duo[2];
    int     _locked = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class imgsink;
class acamera
{

public:
    acamera(const dims_t& wh, const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~acamera();

    virtual bool init(const dims_t&)=0;
    virtual size_t get_frame(imglayout_t& i)=0;
    virtual bool spin()=0;
    const std::string& name()const { return _name; }
    void set_peer(imgsink* peer);
    imgsink* peer(){return   _peer;}
    const uint8_t* getm(int& w, int& h, int& sz);
    const event_t&  proc_events(const imglayout_t& img);
    void  clean_events();
    void  init_frames();

protected:
    camevents       _mt;
    std::string     _name;
    dims_t          _img_size;
    std::string     _location;
    int             _fps;
    imgsink*        _peer = nullptr;
    uint8_t         _predicate = 0;
    DuoFrame*       _frames = nullptr;
};

#endif // ACAMERA_H
