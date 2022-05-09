#include <stdio.h>
#include <string.h>
#include "rtspcam.h"
#include "cbconf.h"
#include "rtspproto.h"

extern bool __alive;

#define OUT_STREAM  std::cerr

///////////////////////////////////////////////////////////////////////////////////////////////////
rtspcam::rtspcam(const dims_t& wh,const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(wh,name,loc,n)
{
    _url = loc;
    TRACE()<<_url<<"\n";
    const std::string& auth = n["login"].value(0);
    _user      = n["login"].value(1);
    size_t   ls = _url.find_last_of("/");
    _uri = _url.substr(ls+1);
    const std::string& pipe = n["pipe"].value();
    if(!pipe.empty())
    {
        TRACE()<< "PIPE FILE " << pipe << "\n";
        _pipa = new pipiefile(pipe.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
rtspcam::~rtspcam()
{
    delete _pipa;
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void  rtspcam::thread_main()
{
    rtspproto   p;
    bool rv = p.cmd_play(_url.c_str(), _user);

    while(!this->is_stopped() && __alive && rv)
    {
        do{
            AutoLock al(&_mut);

            rv = p.spin(_frames->write());
        }while(0);
        if (rv==false)
        {
            p.stop();
            ::sleep(1);
            p.cmd_play(_url.c_str(), _user);
            ::sleep(1);
        }
        ::usleep(20000);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t rtspcam::get_frame(imglayout_t& i)
{
    size_t         leng = 0;
    AutoTryLock    a(&_mut);

    if(a.locked())
    {
        Frame& frame = _frames->read();

        i._caml      = 0;
        if(frame.is_ready())
        {
            _frames->flip();
            i._camf      = eNOTJPG;
            i._dims      = frame._wh;
            i._caml = frame.ptr(&i._camp);
            if(_pipa)
            {
                TRACE() << "piping "  <<frame.length() << " bytes\n";
                _pipa->stream(frame.buffer(),frame.length());
            }
        }
    }
    return leng;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool rtspcam::spin()
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool rtspcam::init(const dims_t&)
{
    init_frames();
    return start_thread()==0;
}

