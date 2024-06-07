///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CLIJSOCK_H
#define CLIJSOCK_H

#include <stddef.h>
#include <stdint.h>
#include "tcpwebsock.h"

class CliJpegSock : public TcpWebSock
{
public:
    CliJpegSock(RawSock&,const LiFrmHdr& h);
    virtual ~CliJpegSock();
    virtual int snd(const uint8_t* b,size_t room,uint32_t extra);

private:

};

#endif // CLIJSOCK_H
