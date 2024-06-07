
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
    _cacheintl    = CFG["webcast"]["cache"].to_int(2);

    if(!_cache.empty())  {
        if(_cache.back()!='/')
            _cache+="/";
        std::string sys = "mkdir -p ";
        sys+=_cache;
        ::system(sys.c_str());
        _cache += name; _cache += ".cache";
	if(::access(_cache.c_str(),0)==0)
	{
		_cached = _maxcache;
	}
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
    this->stop_thread(); delete[] _send_buf;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
bool webcast::stream(const uint8_t* pb,
                     size_t len,
                     const dims_t& imgsz,
                     const std::string& name,
                     const event_t& event,
                     EIMG_FMT eift, time_t now)
{
    if(len){

	if(1)
	{
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
	        _noframe     = 0;
        	::strncpy(ph->camname,name.c_str(),sizeof(LiFrmHdr::camname));
	}

        if(_hasevents == 0)
        {
            _hasevents = (event.predicate & EVT_KEEP_ALIVE);
            if(_hasevents)
            {
                TRACE() << " Camera has events ";
            }
        }

        // while we connect we can loose some frames
        if(_hasevents && !_casting && _cached < _maxcache)
        {
            if(!_cache.empty())
            {
                if(now-_last_frame > _cacheintl &&
                    _cached < _maxcache)
                {
	  	     AutoLock a(&_mut);
		    _last_frame = now;
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
    if(ph->event.predicate & EVT_KEEP_ALIVE)
    {
        TRACE()<< "caching frame [][][]"<< ph->index <<"\n";

        FILE* pf = ::fopen(_cache.c_str(),"ab");
        if(pf==nullptr)
            pf = ::fopen(_cache.c_str(),"wb");
        if(pf){
            ::fwrite(_frame.buff(iframe),1,_frame.frm_len(iframe),pf);
            ::fclose(pf);
            ++_cached;
        }
        _last_cache_time = time(0);
    }
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

    signal(SIGPIPE, SIG_IGN);


    ::strcpy(url, CFG["webcast"]["server"].value().c_str());
    parse_url(url, scheme,
              sizeof(scheme), host, sizeof(host),
              &port, path, sizeof(path));
    ::strcat(url,_name.c_str());
    _ctime = time(0);

    while(!this->osthread::is_stopped() && __alive)
    {
        _send_time = _ctime;
        do{
            AutoLock a(&_mut);
            hasevents = _hasevents;
            _hasevents = 0;
            TRACE() << "Thread events: " << hasevents << "\n";
        }while(0);

        if(time(0)-_ctime > _pool_intl  || hasevents )
        {
            TRACE()<< "Go streaming ----- \n";
            _go_streaming(host, port);
            TRACE()<< "Done streaming ----- \n";
            _ctime = time(0);
            _send_time = _ctime;
            if(::access(_cache.c_str(),0) == 0 )
            {
		_get_cached();
                TRACE()<< "     Go caching \n";
                _send_cache(host, port);
                TRACE()<< "     Done caching \n";
	        _set_cached();
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
bool webcast::_send_all_buffer(const uint8_t* data_,size_t dlen_)
{
    int by=_s.sendall((const uint8_t*)data_, (int)dlen_);
    if(by!=(int)dlen_)
    {
        TRACE() << "Socket Error "<< dlen_ <<" bytes != "<< by <<
            "sent , errno:"<< errno <<"\n";
        return false;
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////////////////////////////////
int webcast::_sign_in(const LiFrmHdr& hdr)
{
    LiFrmHdr        hcopy = hdr;
    int             by;

    hcopy.len = 0;
    hcopy.random = rand();

    _enc.encrypt(hcopy.random, hcopy.challange);
    int val = _enc.decrypt(hcopy.challange);
    assert(val == hcopy.random);

    by = _send_all_buffer((uint8_t*)&hcopy, sizeof(hcopy) );
    ::msleep(32);
    return by;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
size_t webcast::_send_buf_do(LiFrmHdr* ph, const uint8_t* img,  size_t len)
{
    if(_send_buf==nullptr)
    {
        _lastlen = sizeof(LiFrmHdr) * ph->len + 16384;
        _send_buf = new uint8_t[_lastlen];

    }
    if(sizeof(LiFrmHdr)+len <= _lastlen)
    {
        ::memcpy(_send_buf, ph, sizeof(LiFrmHdr));
        ::memcpy(_send_buf + sizeof(LiFrmHdr), img, len);
    }
    else
    {
        TRACE()<< "frame to big\n";
    }
    return sizeof(LiFrmHdr)+len;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
bool webcast::_go_streaming(const char* host, int port)
{
    int             by = 1;
    int             frm_intl = 1000 / (1+_cast_fps);
    int             iframe;
    LiFrmHdr*       ph = nullptr;

    _noframe = 0;
    _s.destroy();
    //_s.pre_set(32768,1024);
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
                ph = _frame.hdr(iframe);
                by = _sign_in(*ph);
            }while(0);

            while(by>0 && _s.isopen() && __alive && _noframe < MAX_NO_FRM_MS)
            {
                _send_time = time(0);
                do{
                    AutoLock a(&_mut);

                    iframe = !_iframe;
	        }while(0);

                    //IN_SYNC();
                    ph = _frame.hdr(iframe);
                    if(ph->len)
                    {
                        by = _send_buf_do(ph, _frame.img(iframe), ph->len);

                        if(false == _send_all_buffer(_send_buf, by))
                        {
                            TRACE()<< "frame !sent \n";
                            goto DONE;
                        }
                        TRACE()<< "frame sent " <<  by << " bytes \n";
                        _casting = true;
                        _noframe = 0;
                    }
                    else
                    {
                        _noframe += frm_intl;
                    }
                    ph->len = 0;
                    _send_time = time(0);
                msleep(1+frm_intl);
            }
        }
    }
DONE:
    do{
        AutoLock a(&_mut);
        _casting = false;
    }while(0);
    _s.destroy();
    return true;
}

void webcast::_send_cache(const char* host, int port)
{
    int         by;

    AutoLock    a(&_mut);

    if(::access(_cache.c_str(),0)==0)
    {
        LiFrmHdr  hdr;
        FILE* pf = ::fopen(_cache.c_str(),"rb");
        if(pf)
        {
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
                    int once=false;
                    while(!::feof(pf))
                    {
                        if(::fread(&hdr, 1, sizeof(hdr), pf))
                        {

                            if(hdr.event.predicate & EVT_KEEP_ALIVE)
                            {
                                int sz = ::fread(_frame.img(0), 1, hdr.len, pf);
                                if(hdr.len==(uint32_t)sz)
                                {
                                    if(once==false)
                                    {
                                        _sign_in(hdr);
                                        once=true;
                                    }

                                    TRACE()<< "-->sending cached frame: "<< hdr.index << ", len:" <<
                                        hdr.len <<
                                        " movepix:" << hdr.event.movepix <<
                                        " predicate:"
                                            << std::hex << int(hdr.event.predicate)
                                            << std::dec << "\n";
                                    by = _send_buf_do(&hdr, _frame.img(0), hdr.len);
                                    if(false == _send_all_buffer(_send_buf, by))
                                    {
                                        goto DONE;
                                    }
                                    _cached--;
                                }
                                else
                                {
                                    TRACE()<< "-->header in cached file is invalid\n";
                                    _cached = 0;
                                    goto DONE;
                                }
                            }
                        }
                        if(::feof(pf))
                        {
                            _cached=0;
                            break;
                        }
                    }
                    _s.destroy();
                }
            DONE:
                ::fclose(pf);
            }
            if(_cached==0)
            {
		TRACE() << "removing cache file \n";
                ::unlink(_cache.c_str());
            }
        }
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

void webcast::_get_cached()
{
	FILE* pf = fopen ("/tmp/lilicached","rb");
	if(pf){
		::fread(&_cached,sizeof(_cached),1,pf);
		::fclose(pf);
		TRACE() << "getting cache" <<  _cached << "\n";
	}
}

void webcast::_set_cached()
{
	FILE* pf = fopen ("/tmp/lilicached","wb");
	if(pf){
		TRACE() << "saving cache" <<  _cached << "\n";
		::fwrite(&_cached,sizeof(_cached),1,pf);
		::fclose(pf);
	}
}


