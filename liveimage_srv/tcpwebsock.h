///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TCPWEBSOCK_H
#define TCPWEBSOCK_H

#include <stddef.h>
#include <stdint.h>
#include "rawsock.h"
class FrameQueue;
class TcpWebSock : public RawSock
{
public:
    TcpWebSock(RawSock&,const LiFrmHdr& h,STYPE t=RawSock::CLIENT);
    virtual ~TcpWebSock();

    virtual int snd(const uint8_t* b,size_t room,uint32_t extra);
    virtual bool destroy(bool be=true);

protected:
    bool    _headered = false;
};

#endif // TCPWEBSOCK_H
