#include "webcast.h"
#include "cbconf.h"
#include "sockserver.h"
#include "strutils.h"
#include "lilitypes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
#define EXTRA_SPACE     4096
#define MAX_NO_FRM_MS	4000
extern bool __alive;

// EVT_KKEP_ALIVE

/////////////////////////////////////////////////////////////////////////////////////////////////
webcast::webcast(const std::string& name):imgsink(name)
{
    ::memset(&_hdr,0,sizeof(_hdr));
    _hdr.magic   = JPEG_MAGIC;

    _hdr.insync  = CFG["webcast"]["insync"].to_int();
    _cast_fps    = CFG["webcast"]["cast_fps"].to_int();
    _pool_intl   = CFG["webcast"]["pool_intl"].to_int();

    if(_pool_intl<6)_pool_intl=10;
    if(_cast_fps>30) _cast_fps=30;
    else if(_cast_fps<1)_cast_fps=1;
}

////////////////////////////////////////////////////////////////////////////////////////////////
webcast::~webcast()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
    delete[] _frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void webcast::stream(const uint8_t* pb, size_t len, const dims_t& imgsz,
                     const std::string& name, const event_t& event, EIMG_FMT eift)
{
    if(len){
        AutoLock a(&_mut);

        if(_frame==nullptr){
            len += EXTRA_SPACE;
            _frame = new uint8_t[len];
            _buffsz = len + EXTRA_SPACE;
        }
        if(len > _buffsz){
            delete[] _frame;
            len += EXTRA_SPACE;
            _frame = new uint8_t[len];
            TRACE() << "renew " << len << "\n";
            _buffsz = len + EXTRA_SPACE;
        }
        ::memcpy(_frame, pb, len);
        _hdr.wh[0]   = imgsz.x;
        _hdr.wh[1]   = imgsz.y;
        _hdr.len     = len;
        _hdr.event   = event;
        _hdr.format  = eift;
        ::strncpy(_hdr.camname,name.c_str(),sizeof(_hdr.camname));
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void webcast::thread_main()
{
    time_t          ctime = 0;
    tcp_cli_sock    cli;
    char            scheme[8];
    int             port = 80;
    char            host[32];
    char            path[64];
    char            url[256];
    int             event_pred;
    int             frame_len;

    //signal(SIGPIPE, SIG_IGN);
    ::strcpy(url, CFG["webcast"]["server"].value().c_str());
    parse_url(url, scheme,
              sizeof(scheme), host, sizeof(host),
              &port, path, sizeof(path));

    ::strcat(url,_name.c_str());

    while(!this->osthread::is_stopped() && __alive)
    {
        do{
            AutoLock a(&_mut);
            event_pred = _hdr.event.predicate;
            frame_len = _hdr.len;
        }while(0);

        if((time(0)-ctime > _pool_intl ||
            (frame_len && event_pred & EVT_KKEP_ALIVE)))
        {
            TRACE()<< "Try streaming \n";
            _go_streaming(host, port);
            ctime = time(0);
        }
        msleep(0x1FF);
    }
}

void webcast::kill()
{
    _s.destroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#define CHECK_SEND(data_, dlen_)                                                    \
    by=_s.sendall((const uint8_t*)data_, (int)dlen_);                               \
    if(by!=(int)dlen_)                                                              \
{                                                                                   \
    TRACE() << "SEND ERROR << "<< by <<" bytes sent != "<< dlen_ <<", errno:"<< errno <<"\n";    \
    goto DONE;                                                                      \
    }

//////////////////////////////////////////////////////////////////////////////////////////////////
void webcast::_go_streaming(const char* host, int port)
{
    int             by;
    int             frm_intl = 1000 / (1+_cast_fps);
    bool            needsframe = false;
    LiFrmHdr        jhdr;
    int             noframe = 0;

    _s.destroy();
    _s.pre_set(8912,4096);
    if(_s.create(port))
    {
        _s.set_blocking(1);
        if(_s.try_connect(host, port)){
            ::usleep(0x1FFFF);
        }
        else{
            TRACE() << "cam cannot connect "<< host << port <<"\r\n";
            goto DONE;
        }
        if(_s.isopen())
        {
            do{
                AutoLock a(&_mut);
                TRACE() << "Event move: " << _hdr.event.movepix << "\r\n";
                jhdr = _hdr;
            }while(0);

            jhdr.len = 0;
            jhdr.random = rand();
            _enc.encrypt(jhdr.random, jhdr.challange);
            CHECK_SEND(&jhdr, sizeof(jhdr));
            ::msleep(256);
            while(by && _s.isopen() && __alive && noframe < MAX_NO_FRM_MS)
            {
                if(_hdr.insync)
                {
                    needsframe = false;
                    TRACE() << "WAITING \r\n" ;
                    by = _s.receiveall((uint8_t*)&jhdr, sizeof(jhdr));
                    if(by!=sizeof(jhdr)){
                        TRACE() << "REC ERROR " << by <<", "<< _s.error() << "\n";
                        goto DONE;
                    }
                    if(jhdr.len==0){
                        needsframe=true;
                    }

                    if(needsframe==false){
                        goto END_WILE;
                    }
                }
                _hdr.index++;
                do{
                    AutoLock a(&_mut);
                    if(_hdr.len){
                        CHECK_SEND(&_hdr, sizeof(_hdr));
                        CHECK_SEND(_frame, _hdr.len)
                                noframe = 0;
                    }else{
                        noframe += frm_intl;
                    }
                    _hdr.len = 0;
                }while(0);
END_WILE:
                msleep(1+frm_intl);
            }
        }
    }
DONE:
    AutoLock a(&_mut);
    _hdr.len  = 0;
    _s.destroy();
}

bool webcast::spin()
{
    return true;
}

bool webcast::init(const dims_t&)
{
    return start_thread()==0;
}
