
#include <unistd.h>
#include "webcast.h"
#include "cbconf.h"
#include "sockserver.h"
#include "strutils.h"
#include "lilitypes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
#define EV_TAILS        2  /* send cache */
#define MAX_NO_FRM_MS	5000
#define STUCK_IN_SEND   20
#define WE_TRIED_TO_SEND_CACHE 2
extern bool __alive;

// EVT_KEEP_ALIVE

/////////////////////////////////////////////////////////////////////////////////////////////////
webcast::webcast(const std::string& name):imgsink(name)
{
    _magic       = JPEG_MAGIC;
    _insync      = CFG["webcast"]["insync"].to_int();
    _cast_fps    = CFG["webcast"]["cast_fps"].to_int();
    _pool_intl   = CFG["webcast"]["pool_intl"].to_int();
    _cache       = CFG["webcast"]["cache"].value();
    _maxcache    = CFG["webcast"]["cache"].to_int(1);
    if(!_cache.empty())  {
        if(_cache.back()!='/')
            _cache+="/";
        std::string sys = "mkdir -p ";
        sys+=_cache;
        ::system(sys.c_str());
        _cache += name; _cache += ".cache";
        FILE* pf = ::fopen(_cache.c_str(),"wb");
        ::fclose(pf);
    }

    if(_pool_intl<6)_pool_intl=10;
    if(_cast_fps>30) _cast_fps=30;
    else if(_cast_fps<1)_cast_fps=1;
    _send_time = time(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////
webcast::~webcast()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
bool webcast::stream(const uint8_t* pb,
                     size_t len,
                     const dims_t& imgsz,
                     const std::string& name,
                     const event_t& event,
                     EIMG_FMT eift)
{
    if(len){
        AutoLock a(&_mut);

        _frame.copy(pb, len, _iframe);
        LiFrmHdr* ph = _frame.hdr(_iframe);
        ph->index    = _frmidx++;
        ph->wh[0]    = imgsz.x;
        ph->wh[1]    = imgsz.y;
        ph->len      = len;
        ph->event    = event;
        ph->format   = eift;
        ph->magic    = _magic;
        ph->insync   = _insync;

        _noframe = 0;
        ::strncpy(ph->camname,name.c_str(),sizeof(LiFrmHdr::camname));
        if(_hasevents==0)
            _hasevents = (event.predicate & EVT_KEEP_ALIVE);

        // while we connect we can loose some frames
        if( ((_hasevents && !_casting ) || time(0)-_send_time > STUCK_IN_SEND )
               && _cached < _maxcache )
        {
            if(!_cache.empty())
            {
                static int everysec = time(0);
                if(time(0)-everysec>2)
                {
                    everysec = time(0);
                    _cache_frame(_iframe);
                }
            }
        }
        _iframe = ! _iframe;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void webcast::_cache_frame(int iframe)
{
    LiFrmHdr* ph = _frame.hdr(iframe);
    TRACE()<< "caching frame [][][]"<< ph->index <<"\n";
    ++_cached;
    FILE* pf = ::fopen(_cache.c_str(),"ab");
    ::fwrite(_frame.buff(iframe),1,_frame.frm_len(iframe),pf);
    ::fclose(pf);
    _last_cache_time = time(0);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
void webcast::thread_main()
{
    tcp_cli_sock    cli;
    char            scheme[8];
    int             port = 80;
    char            host[32];
    char            path[64];
    char            url[256];
    int             hasevents;

    ::strcpy(url, CFG["webcast"]["server"].value().c_str());
    parse_url(url, scheme,
              sizeof(scheme), host, sizeof(host),
              &port, path, sizeof(path));
    ::strcat(url,_name.c_str());
    _ctime = time(0);
    _casting = false;
    while(!this->osthread::is_stopped() && __alive)
    {
        _send_time = _ctime;
        do{
            AutoLock a(&_mut);
            hasevents = _hasevents;
            _hasevents = 0;
        }while(0);
        if(time(0)-_ctime > _pool_intl  || hasevents )
        {
            TRACE()<< "     Go streaming \n";
            _casting = false;
            _go_streaming(host, port);
            TRACE()<< "     Done streaming \n";
            _ctime = time(0);
            _send_time = _ctime;
            if(_cached && _last_cache_time - _ctime > WE_TRIED_TO_SEND_CACHE)
            {
                _go_streaming(host, port);
                _cached = 0;
            }
        }
        if(!hasevents){
            ::msleep(MAX_NO_FRM_MS);
        }
        ::msleep(32);
    }
}

void webcast::kill()
{
    _s.destroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#define CHECK_SEND(data_, dlen_)                             \
    by=_s.sendall((const uint8_t*)data_, (int)dlen_);        \
    if(by!=(int)dlen_){                                      \
    TRACE() << "Socket Error "<< dlen_ <<" bytes != "<< by <<"sent , errno:"<< errno <<"\n"; \
    goto DONE;                                           \
    }
//////////////////////////////////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////////////////////////////////
#define IN_SYNC()    if(_frame.hdr(iframe)->insync)                 \
{         \
    needsframe = false;         \
    TRACE() << "WAITING \r\n" ;         \
    by = _s.receiveall((uint8_t*)&jhdr, sizeof(jhdr));         \
    if(by != sizeof(jhdr)){         \
        TRACE() << "REC ERROR " << by <<", "<< _s.error() << "\n";         \
        goto DONE;         \
    }         \
    if(jhdr.len==0){         \
        needsframe=true;         \
    }         \
    if(needsframe==false){         \
        goto END_WILE;         \
    }         \
}
//////////////////////////////////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////////////////////////////////
int webcast::_sign_in()
{
    LiFrmHdr        jhdr;
    int             by;

    do{
        AutoLock a(&_mut);
        jhdr = *_frame.hdr(!_iframe);
    }while(0);

    jhdr.len = 0;
    jhdr.random = rand();
    _enc.encrypt(jhdr.random, jhdr.challange);
    CHECK_SEND(&jhdr, sizeof(jhdr) );
DONE:
    ::msleep(256);
    return by;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
bool webcast::_go_streaming(const char* host, int port)
{
    int             by = 1;
    int             frm_intl = 1000 / (1+_cast_fps);
    bool            needsframe = false;
    int             iframe;
    LiFrmHdr*       ph;
    LiFrmHdr        jhdr;

    _noframe = 0;
    _s.destroy();
    _s.pre_set(32768,1024);
    if(_s.create(port))
    {
        _s.set_blocking(1);
        if(_s.try_connect(host, port)){
            ::usleep(0x1FF);
        }
        else{
            TRACE() << "cam cannot connect "<< host << port <<"\r\n";
            goto DONE;
        }
        if(_s.isopen())
        {
            do{
                AutoLock a(&_mut);
                iframe = !_iframe;
            }while(0);
            by = _sign_in();
            if(_cached && by && !_cache.empty())
            {
                _send_cache(iframe);
            }
            while(by>0 && _s.isopen() && __alive && _noframe < MAX_NO_FRM_MS)
            {
                _send_time = time(0);
                do{
                    AutoLock a(&_mut);
                    iframe = !_iframe;
                }while(0);
                IN_SYNC();
                ph = _frame.hdr(iframe);
                if(ph->len)
                {
                    CHECK_SEND(ph, sizeof(LiFrmHdr));
                    CHECK_SEND(_frame.img(iframe),ph->len);
                    _noframe = 0;
                }
                else
                {
                    _noframe += frm_intl;
                }
                ph->len = 0;
                ph->event.movepix = 0;
                ph->event.predicate = 0;
                _casting = true;
                _send_time = time(0);
END_WILE:
                msleep(1+frm_intl);
            }
        }
    }
DONE:

    _s.destroy();
    _casting = false;
    return true;
}

void webcast::_send_cache(int iframe)
{
    int         by;
    AutoLock    a(&_mut);
    if(::access(_cache.c_str(),0)==0 && _cached)
    {
        LiFrmHdr  hdr;
        FILE* pf = ::fopen(_cache.c_str(),"rb");
        while(!::feof(pf))
        {
            if(::fread(&hdr, 1, sizeof(hdr), pf))
            {
                CHECK_SEND(&hdr, sizeof(hdr));
                int sz = ::fread(_frame.img(iframe), 1, hdr.len, pf);
                if(hdr.len==(uint32_t)sz){
                    CHECK_SEND(_frame.img(iframe), hdr.len);
                }
                else
                {
                    TRACE()<< "-->header in cached file is invalid\n";
                    goto DONE;
                }
                TRACE()<< "-->sending cached frame "<< hdr.index << "," << hdr.len <<"\n";
            }
            if(::feof(pf)){ break; }
        }
DONE:
        ::fclose(pf); ::msleep(1);
        ::fopen(_cache.c_str(),"w");
        ::msleep(1); ::fclose(pf);
        _cached = 0;
    }
}

bool webcast::spin()
{
    return true;
}

bool webcast::init(const dims_t&)
{
    return start_thread()==0;
}
