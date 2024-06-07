#ifndef RTSPSOCK_H
#define RTSPSOCK_H

#include "connsock.h"
#include "rtpudp.h"
#include "clisock.h"
#include "framequeue.h"

class CliRtspSock : public CliSock
{
public:
    CliRtspSock(ConnSock&, const optionsmap& h, int uc, int us, size_t session);
    virtual ~CliRtspSock();

    virtual int     snd(const uint8_t* b, size_t room, uint32_t extra=0);
    virtual int     snd(const char* b, size_t room, uint32_t extra=0);
    virtual bool    destroy();
    size_t          session()const{return _session;}
    virtual bool    isopen();
    int             transfer(const std::vector<ConnSock*>& clis);

private:
    int         _udpportc;
    int         _udpports;
    int         _seldom=0;
    size_t      _session;
    RtpUdp*     _udppush;
    uint32_t    _sequence=0;
    std::string _addr;
    FrameQueue  _q;
};

#endif // RTSPSOCK_H
