#ifndef ACAMERA_H
#define ACAMERA_H

#include <string>
#include <chrono>
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

    Frame*  to_load(){
        if(!_duo[_locked].is_ready()){
            return &_duo[_locked];
        }
        return nullptr;
    }

    Frame*  to_stream(){
        if(_duo[_locked].is_ready())
            return &_duo[_locked];
        else if(_duo[!_locked].is_ready())
            return &_duo[!_locked];
        return nullptr;
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
    void set_motion(const dims_t& mohilo, int pixnoise, int pixdiv, int imgscale);
    void get_motion(dims_t& mohilo, int& pixnoise, int& pixdiv, int& imgscale);
    void  save_events(const imglayout_t& img, const std::string& folder);
    void count_fps();
    virtual void set_fps(int fps);
protected:
    camevents       _mt;
    std::string     _name;
    dims_t          _img_size;
    std::string     _location;
    int             _fps;
    imgsink*        _peer = nullptr;
    uint8_t         _predicate = 0;
    DuoFrame*       _frames = nullptr;
    int             _real_fps = 0;
    std::chrono::steady_clock::time_point _tp;
};

#endif // ACAMERA_H
