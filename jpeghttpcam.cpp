

#include "jpeghttpcam.h"
#include "sock.h"
#include "webcast.h"


extern bool __alive;
#define SIG_LEN 16

#define GET_HDRGET "GET %s HTTP/1.1\r\n"            \
    "Host: %s\r\n"                                  \
    "User-Agent: liveimage 1.0\r\n"                 \
    "Accept: text/html,application/xhtml+xml\r\n"   \
    "Connection: close\r\n"                         \
    "Pragma: no-cache\r\n"                          \
    "Cache-Control: no-cache\r\n\r\n"

jpeghttpcam::jpeghttpcam(const dims_t& wh,
                         const std::string& name,
                         const std::string& loc,
                         const Cbdler::Node& n):acamera(wh,name,loc,n)
{
    _format         = n["format"].value() == "image/jpg"  ? eFJPG : eNOTJPG;
    _imgsz.x        = n["imgsz"].to_int(0);
    _imgsz.y        = n["imgsz"].to_int(1);
    _msleep         = 1000 / n["fps"].to_int();
    if(_msleep > 1000)_msleep = 1000;
    else if(_msleep<500)_msleep=500;
}

jpeghttpcam::~jpeghttpcam()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
}

bool jpeghttpcam::init(const dims_t&)
{
    _frame[0].set_len(0);
    _frame[1].set_len(0);
    return start_thread();
}

size_t jpeghttpcam::get_frame(imglayout_t& i)
{
    AutoTryLock     al(&_mut);
    size_t          len = 0;
    int             ifrm = !_ifrm;

    if(al.locked())
    {
        if(_frame[ifrm].length())
        {
            i._caml   = _frame[ifrm].ptr(&i._camp);
            i._camf   = ::is_jpeg(i._camp, i._caml) ? _format : eNONE;
            i._dims.x = _imgsz.x;
            i._dims.y = _imgsz.y;
        }
        else
        {
            i._caml = 0;
        }
        _frame[ifrm].set_len(0);
        _ifrm = !_ifrm;
    }
    return len;
}


bool jpeghttpcam::spin()
{
    return true;
}

void jpeghttpcam::thread_main()
{
    tcp_cli_sock    s;
    bool            isjpg = false;
    char*           eoh;
    int             content_lengh;
    int             dummy;
    int             hdrlen;     // = (eoh  -  rec) + 4;
    int             over_flow;  // = hlen - hdrlen;
    char            *scontent_lengh;
    char            req[512];
    char            rec[512];
    int             iport = 80;
    char            url[256];
    char            scheme[8];
    char            ihost[32];
    char            ipath[64];

    ::strcpy(url, _location.c_str());
    parseURL(url, scheme,
             sizeof(scheme), ihost, sizeof(ihost),
             &iport, ipath, sizeof(ipath));

    while(!this->is_stopped() && __alive)
    {
        ::usleep(_msleep << 10);
        if(s.isopen()){

            AutoLock    al(&_mut);
            s.select_receive(_frame[_ifrm].buffer(), _frame[_ifrm].capa(), 4000);
            _frame[_ifrm].set_len(0);
            s.destroy();
        }
        s.create(iport);

        if(s.isopen()  && s.try_connect(ihost, iport)){
            int l = ::sprintf(req, GET_HDRGET, ipath, ihost);
            dummy = s.sendall(req, l);
            if(dummy != l){
                continue;
            }

            int hlen  = s.receive(rec, sizeof(rec)-32);
            if(hlen <= 0){
                continue;
            }

            rec[hlen]='\0';
            eoh = ::strstr(rec,"\r\n\r\n");
            if(eoh == nullptr){
                continue;
            }
            hdrlen = (eoh  -  rec) + 4;
            over_flow = hlen - hdrlen;
            isjpg = true;
            do{
                AutoLock    al(&_mut);
                _frame[_ifrm].set_len(0);
                if(over_flow>0){
                    const uint8_t *pafter = (const uint8_t*)(rec + hdrlen);
                    _frame[_ifrm].copy(pafter, 0, over_flow);
                    isjpg = ::is_jpeg(_frame[_ifrm].buffer(), over_flow);
                }
                if(isjpg==false){
                    continue;
                }
                scontent_lengh = strstr(rec,"Content-Length: ");
                if(scontent_lengh==0){
                    continue;
                }
                content_lengh = ::atoi(scontent_lengh + 16);
                content_lengh -= over_flow;
                if(content_lengh <= 0)
                {
                    continue;
                }
                if(_frame[_ifrm].realloc(content_lengh + over_flow + SIG_LEN)==false)
                {
                    TRACE() << "Cannot allocat memory\n.Exiting\n";
                    __alive =false;
                    break;
                }
                hlen = s.receiveall( _frame[_ifrm].buffer(over_flow), content_lengh);
                if(hlen != content_lengh){
                    continue;
                }
                isjpg = ::is_jpeg( _frame[_ifrm].buffer(), content_lengh + over_flow);
                if(isjpg){
                    _frame[_ifrm].set_len(content_lengh + over_flow);
                }else{
                    _frame[_ifrm].set_len(0);
                }
            }while(0);
        }// connected
        s.destroy();
    }//thread loop
}












