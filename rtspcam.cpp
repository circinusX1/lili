#include <stdio.h>
#include <string.h>
#include "rtspcam.h"
#include "cbconf.h"
#include "rtspproto.h"

extern bool __alive;

#define OUT_STREAM  std::cerr

rtspcam::rtspcam(const dims_t& wh,const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(wh,name,loc,n)
{
    _url = loc;
    const std::string& proto = n["rtsp"].value();
    if(proto=="TCP"){
        _transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
        _ontcp = true;
    }
    else
        _transport = "RTP/AVP;unicast;client_port=";
    //_transport = "RAW/RAW/UDP;unicast;client_port=";

    std::string auth = n["login"].value(0);
    if(auth=="BASIC")
        _curl_auth = CURLAUTH_BASIC;
    else if(auth=="DIGEST")
        _curl_auth = CURLAUTH_DIGEST;
    if(auth=="NEGOCIATE")
        _curl_auth = CURLAUTH_NEGOTIATE;
    _user      = n["login"].value(1);
    size_t   ls = _url.find_last_of("/");
    _uri = _url.substr(ls+1);
}

rtspcam::~rtspcam()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
}


void  rtspcam::thread_main()
{
    rtspproto   p;

    pipiefile pif("/tmp/movie.mov");

    p.cmd_play(_url.c_str(), _user);
    while(!this->is_stopped() && __alive)
    {
        p.spin(_frames[_flip]);
        if(_frames[_flip].room() < STEP_LEN)
        {
            TRACE() << "piping \n";
            pif.stream(_frames[_flip].buffer(),_frames[_flip].length());
            _frames[_flip].reset();
        }
        ::usleep(10000);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t rtspcam::get_frame(imglayout_t& i)
{
    size_t          leng = 0;
    AutoTryLock    a(&_mut);

    if(a.locked())
    {
    /*
        Frame& frame = _frames[_flip];
        i._caml = 0;
        i._camf = eNOTJPG;
        if(frame.length())
        {
            i._caml = frame.ptr(&i._camp);
            _flip = !_flip;
        }
*/
    }
    return leng;
}


bool rtspcam::spin()
{
    return true;
}



bool rtspcam::init(const dims_t&)
{
    return start_thread()==0;
}

