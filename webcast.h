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
    virtual void stream(const uint8_t* pb, size_t len, const dims_t& imgsz,
                        const std::string& name, const event_t& event, EIMG_FMT eift);
    void kill();
    virtual bool spin();
    virtual bool init(const dims_t&);

private:
    void _go_streaming(const char* host, int port);

private:

    mutexx           _mut;
    uint8_t*        _frame  = nullptr;
    bool            _headered = false;
    time_t          _last_clicheck=0;
    tcp_cli_sock    _s;
    LiFrmHdr        _hdr;
    size_t          _buffsz=0;
    int             _cast_fps = 1;
    int             _pool_intl = 10;
    uint32_t        _srv_key = 0;
    std::string     _security;
    Encryptor       _enc;
};

#endif // WEBCAST_H
