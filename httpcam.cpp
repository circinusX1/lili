#include "httpcam.h"
#include "sock.h"
#include "webcast.h"
//#include "CImg.h"

extern bool __alive;


#define GET_HDRGET "GET %s HTTP/1.1\r\n"                            \
    "Host: %s\r\n"                                  \
    "User-Agent: liveimage 1.0\r\n"                 \
    "Accept: text/html,application/xhtml+xml\r\n"   \
    "Connection: close\r\n"                         \
    "Pragma: no-cache\r\n"                          \
    "Cache-Control: no-cache\r\n\r\n"

httpcam::httpcam(const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(name,loc,n)
{
    _format         = n["format"].value() == "image/jpg"  ? eFJPG : eFMPG;
    _motionurl      = n["motion"].value(0);
    _motiontoken    = n["motion"].value(1);
    _imgsz.x        = n["imgsz"].to_int(0);
    _imgsz.y        = n["imgsz"].to_int(1);
    _msleep         = 1000 / n["fps"].to_int();
    if(_msleep > 1000)_msleep = 1000;
    else if(_msleep<500)_msleep=500;
}

httpcam::~httpcam()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
}

bool httpcam::init(const dims_t&)
{
    return start_thread();
}

size_t httpcam::get_frame(const uint8_t** pb, EIMG_FMT& fmt, event_t& event)
{
    AutoTryLock    al(&_mut);
    if(_frame[_ifrm].length())
    {
        size_t len = _frame[_ifrm].ptr(pb);
        _ifrm = _ifrm;
        fmt = _format;
        event.movepix = _moved ? 1 : 0;
        event.predicate |= EVT_MOTION;
        return len;
    }
    return 0;
}


bool httpcam::spin(event_t&)
{
    // query the http
    return true;
}

void httpcam::thread_main()
{
    tcp_cli_sock    s;
    tcp_cli_sock    sm;
    char            req[512];
    int             iport = 80;
    char            url[256];
    char            scheme[8];
    char            ihost[32];
    char            ipath[64];
    int             mport = 80;
    char            mhost[32];
    char            mpath[64];

    ::strcpy(url, _location.c_str());
    parseURL(url, scheme,
             sizeof(scheme), ihost, sizeof(ihost),
             &iport, ipath, sizeof(ipath));
    if(!_motionurl.empty())
    {
        ::strcpy(url, _motionurl.c_str());
        parseURL(url, scheme,
                 sizeof(scheme), mhost, sizeof(mhost),
                 &mport, mpath, sizeof(mpath));

    }else{
        mport = 0;
    }

    while(!this->is_stopped() && __alive)
    {
        if(mport){
            sm.create(mport);
        }
        s.create(iport);

        if(s.isopen() && s.try_connect(ihost, iport)){
            int l = ::sprintf(req, GET_HDRGET, ipath, ihost);
            //TRACE() << req;
            s.sendall(req, l);

            AutoLock    al(&_mut);
            _frame[_ifrm].set_len(0);
            int hlen  = s.receive(url, sizeof(url));
            if(hlen > 0)
            {
                char* eoh = ::strstr(url,"\r\n\r\n");
                if(eoh)
                {

                    //TRACE() << url << "\r\n";
                    hlen = hlen-(eoh-url+4);
                    if(hlen){
                        ::memcpy(_frame[_ifrm].buffer(), eoh+4, hlen);
                    }
                    int icl = (int)_frame[_ifrm].capa()-hlen;
                    char* pcl = strstr(url,"Content-Length: ");
                    if(pcl){
                        icl = ::atoi(pcl+16) - hlen;
                        if(icl<=0)
                            icl = (int)_frame[_ifrm].capa()-hlen;
                    }
                    int bytes = s.receive(_frame[_ifrm].buffer(hlen),
                                          icl);
                    if(bytes>0)
                    {
                        //TRACE() << bytes << "\r\n";
                        _frame[_ifrm].set_len(bytes);
                    }
                }
            }
            s.destroy();
        }
        if(sm.isopen() && sm.try_connect(mhost, mport)){
            int l = ::sprintf(req, GET_HDRGET, mpath, mhost);
            int got = 0;
            sm.sendall(req, l);
            while(got < 2000)
            {
                int ir = sm.receive(req,sizeof(req)-1);
                if(ir>0)
                {
                    got += ir;

                    if(::strstr(req,_motiontoken.c_str()))
                    {
                        TRACE() << "MOTION " << req << "\n";
                        _moved = true;
                        break;
                    }
                }
                else
                    break;
            }
            sm.destroy();
        }
        ::usleep(1000*_msleep);
    }
}

void httpcam::to_gray()
{
    if(_imgsz.x){


    }
}











