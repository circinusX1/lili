#include <stdio.h>
#include <string.h>
#include "rtspcam.h"
#include "cbconf.h"
#include "avlibrtsp.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
extern bool __alive;

///////////////////////////////////////////////////////////////////////////////////////////////////
rtspcam::rtspcam(const dims_t& wh,const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(wh,name,loc,n)
{
    _url = loc;
    TRACE()<<_url<<"\n";
    const std::string& auth = n["login"].value(0);
    _user = n["login"].value(1);
    size_t ls = _url.find_last_of("/");
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
#ifdef WITH_AVLIB_RTSP
    _avlib_rtsp();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t rtspcam::get_frame(imglayout_t& i)
{
    size_t         leng = 0;
    AutoTryLock    a(&_mut);

    if(a.locked())
    {
        Frame* frame = _frames->to_stream();
        if(frame)
        {
            i._caml      = 0;
            i._camf        = e422;
            i._dims        = _img_size;
            _frames->flip();
            leng = i._caml = frame->ptr(&i._camp);
            if(_pipa)
            {
                TRACE() << "piping "  << frame->length() << " bytes\n";
                _pipa->stream(frame->buffer(),frame->length());
            }
        }
    }
    return leng;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void rtspcam::_avlib_rtsp()
{
    avrlibrtsp   p(this->_img_size);
    bool         okay = p.init(_url,_user, _img_size);

    if(okay)
    {
        while(!this->is_stopped() && __alive && okay)
        {
            do{
                AutoLock    a(&_mut);
                if(false == p.spin(_frames->to_load()))
                {
                    p.destroy();
                    ::sleep(1);
                    okay = p.init(_url, _user, _img_size);
                }

            }while (0);
            ::usleep(20000);
        }
    }
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

