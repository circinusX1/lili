#ifndef WEBCAST_H
#define WEBCAST_H

#include "os.h"
#include "sock.h"
#include "sockserver.h"
#include "encrypter.h"
#include "imgsink.h"
#include "lilitypes.h"



class webcast : public osthread, public imgsink
{
    enum eHOW{
        e_offline,
        e_tcp,
        e_udp,
        e_rtp,
    };

public:
    webcast(const std::string& name);
    virtual ~webcast();
    virtual void thread_main();
    virtual bool stream(const uint8_t* pb, size_t len, const dims_t& imgsz,
                        const std::string& name, const event_t& event, EIMG_FMT eift, time_t ct);
    void kill();
    virtual bool spin();
    virtual bool init(const dims_t&);
    const std::string cache(){return _cache;}
private:
    bool _go_streaming(const char* host, int port);
    void _cache_frame( int iframe);
    int  _sign_in(const LiFrmHdr& hdr);
    void _send_cache(const char* host, int port);
    size_t _send_buf_do(LiFrmHdr* ph, const uint8_t* img,  size_t len);
    void _set_cached();
    void _get_cached();
    bool _send_all_buffer(const uint8_t* data_,size_t dlen_);

private:
    uint8_t         *_send_buf=nullptr;
    size_t          _lastlen = 0;
    mutexx          _mut;
    WebDblFrame     _frame;
    int8_t          _iframe = 0;
    bool            _headered = false;
    time_t          _last_clicheck=0;
    time_t          _last_frame=0;
    tcp_cli_sock    _s;
    size_t          _buffsz=0;
    int             _cast_fps = 1;
    int             _pool_intl = 10;
    uint32_t        _srv_key = 0;
    std::string     _security;
    Encryptor       _enc;
    bool            _casting = false;
    int             _hasevents = 0;
    std::string     _cache;
    int             _maxcache;
    int             _cacheintl=1000;
    int             _cached = 0;
    int             _noframe = 0;
    int             _frmidx = 0;
    uint32_t        _magic;         //       = JPEG_MAGIC;
    uint8_t         _insync;        //      = CFG["webcast"]["insync"].to_int();
    time_t          _ctime=0;
    time_t          _last_cache_time = 0;
    time_t          _send_time = 0;

};

#endif // WEBCAST_H
