#include "webcast.h"
#include "cbconf.h"
#include "sockserver.h"
#include "strutils.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
#define EXTRA_SPACE 2048
extern bool __alive;

/////////////////////////////////////////////////////////////////////////////////////////////////
WebCast::WebCast(int ifmt)
{
    _hdr.magic   = JPEG_MAGIC;

    _hdr.record  = CFG["webcast"]["record"].to_int();
    _hdr.format  = (ifmt);
    _hdr.insync  = CFG["webcast"]["insync"].to_int();

    _cast_fps    = CFG["webcast"]["cast_fps"].to_int();
    _pool_intl   = CFG["webcast"]["pool_intl"].to_int();

    if(_pool_intl<6)_pool_intl=10;
    if(_cast_fps>60) _cast_fps=60;
    else if(_cast_fps<1)_cast_fps=1;
}

////////////////////////////////////////////////////////////////////////////////////////////////
WebCast::~WebCast()
{
    this->stop_thread();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void WebCast::stream_frame(uint8_t* pjpg, size_t length, int event, int w, int h)
{
    AutoLock a(&_mut);

    if(_frame==nullptr){
        _frame = new uint8_t[length + EXTRA_SPACE];
        std::cout << "new " << length << "\n";
        _buffsz = length + EXTRA_SPACE;
    }
    if(length > _buffsz){
        delete[] _frame;
        _frame = new uint8_t[length + EXTRA_SPACE];
        std::cout << "renew " << length << "\n";
        _buffsz = length + EXTRA_SPACE;
    }
    ::memcpy(_frame, pjpg, length);
    _hdr.wh[0]   = w;
    _hdr.wh[1]   = h;
    _hdr.len     = length;
    _hdr.event = event;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void WebCast::thread_main()
{
    time_t          ctime = 0;
    tcp_cli_sock    cli;
    bool            uconnect = false;
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

    while(!this->OsThread::is_stopped() && __alive)
    {
        if((time(0)-ctime > _pool_intl && _hdr.len) ||
                (_hdr.event>0 && _hdr.record) || uconnect)
        {
            _go_streaming(host, path+1, port);
            ctime = time(0);
        }
        msleep(0xFF);
    }
}

void WebCast::kill()
{
    _s.destroy();
}

#define CHECK_SEND(data_, dlen_)                                \
    by=_s.sendall((const uint8_t*)data_, (int)dlen_);          \
    if(by!=(int)dlen_)                                               \
{                                                           \
    std::cout << "SEND ERROR << "<< by <<" bytes sent, errno:"<< errno <<"\r\n"; \
    goto DONE;                                              \
    }

void WebCast::_go_streaming(const char* host, const char* camname, int port)
{
    int             by;
    int             frm_intl = 1000 / (1+_cast_fps);
    bool            needsframe = false;
    LiFrmHdr        jhdr;

    _s.destroy();
    if(_s.create(port))
    {
        int set = 1;
//        ::setsockopt(_s.sock(), 
//		     SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

        _s.set_blocking(1);
        if(_s.try_connect(host, port)){
            std::cout << "cam connected "<<host<< port<<"\r\n";
        }
        else{
            std::cout << "cam cannot connect "<<host<< port<<"\r\n";
            goto DONE;
        }
        if(_s.isopen())
        {
            //////////////////////////////////// connect
            std::cout << "cam stream really connected \r\n";
            ::strncpy(_hdr.camname, camname, sizeof(_hdr.camname));
            do{
                AutoLock a(&_mut);
                std::cout << "init conn with event " << _hdr.event << "\r\n";
                jhdr = _hdr;
            }while(0);

            //////////////////////////////////// header
            jhdr.len = 0;
            jhdr.random = rand();
            _enc.encrypt(jhdr.random, jhdr.challange);
            int g = _enc.decrypt(jhdr.challange);
            CHECK_SEND(&jhdr, sizeof(jhdr));

            msleep(1000);
            while(by && _s.isopen() && __alive)
            {
                ////////////////////////////////////////// in sync
                if(_hdr.insync)
                {
                    needsframe = false;
                    std::cout << "WAITING \r\n" ;
                    _s.set_blocking(1);
                    by = _s.receiveall((uint8_t*)&jhdr, sizeof(jhdr));
                    if(by!=sizeof(jhdr)){
                        std::cout << "REC ERROR " << by <<", "<< _s.error() << "\n";
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
                    }
                    std::cout << "sent " << _hdr.len << "\n" ;
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
    std::cout << "socked close\r\n";
}

