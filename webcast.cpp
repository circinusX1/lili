#include "webcast.h"
#include "cbconf.h"
#include "sockserver.h"
#include "strutils.h"
#include "lilitypes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
#define EXTRA_SPACE 2048
extern bool __alive;

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
    AutoLock a(&_mut);

    if(_frame==nullptr){
        _frame = new uint8_t[len + EXTRA_SPACE];
        _buffsz = len + EXTRA_SPACE;
    }
    if(len > _buffsz){
        delete[] _frame;
        _frame = new uint8_t[len + EXTRA_SPACE];
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

    //signal(SIGPIPE, SIG_IGN);
    ::strcpy(url, CFG["webcast"]["server"].value().c_str());
    parseURL(url, scheme,
             sizeof(scheme), host, sizeof(host),
             &port, path, sizeof(path));

    ::strcat(url,_name.c_str());

    while(!this->osthread::is_stopped() && __alive)
    {
        if((time(0)-ctime > _pool_intl || (_hdr.len && _hdr.event.predicate & EVT_KKEP_ALIVE)))
        {
            _go_streaming(host, port);
            ctime = time(0);
        }
        msleep(0xFF);
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
    TRACE() << "SEND ERROR << "<< by <<" bytes sent, errno:"<< errno <<"\r\n";    \
    goto DONE;                                                                      \
    }

void webcast::_go_streaming(const char* host, int port)
{
    int             by;
    int             frm_intl = 1000 / (1+_cast_fps);
    bool            needsframe = false;
    LiFrmHdr        jhdr;
    int             noframe = 0;

    _s.destroy();
    if(_s.create(port))
    {
        _s.set_blocking(1);
        if(_s.try_connect(host, port)){
            TRACE() << "cam connected "<< host << port <<"\r\n";
        }
        else{
            TRACE() << "cam cannot connect "<< host << port <<"\r\n";
            goto DONE;
        }
        if(_s.isopen())
        {
            //////////////////////////////////// connect
            TRACE() << "cam stream really connected \r\n";

            do{
                AutoLock a(&_mut);
                TRACE() << "init conn with event " << _hdr.event.movepix << "\r\n";
                jhdr = _hdr;
            }while(0);

            //////////////////////////////////// header
            jhdr.len = 0;
            jhdr.random = rand();
            _enc.encrypt(jhdr.random, jhdr.challange);
            //int g = _enc.decrypt(jhdr.challange);
            CHECK_SEND(&jhdr, sizeof(jhdr));

            msleep(1000);
            while(by && _s.isopen() && __alive)
            {
                ////////////////////////////////////////// in sync
                if(_hdr.insync)
                {
                    needsframe = false;
                    TRACE() << "WAITING \r\n" ;
                    _s.set_blocking(1);
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
                        ///////////////////////////////////////////////  header
                        CHECK_SEND(&_hdr, sizeof(_hdr));
                        ///////////////////////////////////////////////  frame
                        CHECK_SEND(_frame, _hdr.len)
                        noframe = 0;
                    }else{
                        noframe += frm_intl;
                    }
                    TRACE() << "sent " << _hdr.len << "\n" ;
                    _hdr.len = 0;
                }while(0);
END_WILE:
                msleep(1+frm_intl);
//                if(noframe > _pool_intl*1000){
  //                  TRACE() << "no nwe frames \n" ;
    //                break;
      //          }
            }
        }
    }
DONE:
    AutoLock a(&_mut);
    _hdr.len  = 0;
    _s.destroy();
    TRACE() << "socked close\r\n";
}

bool webcast::spin()
{
    return true;
}

bool webcast::init(const dims_t&)
{
    return start_thread()==0;
}
