///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TCPJPGMPARTCAM_H
#define TCPJPGMPARTCAM_H

#include "tcpcamcli.h"

#define    JPEG_MAGIC       0x12345678


class TcpJpgMpartCam : public TcpCamCli
{
public:
    TcpJpgMpartCam(RawSock& ,
                   const LiFrmHdr& h,const char* pextra);

    virtual ~TcpJpgMpartCam();
    virtual int  transfer(const std::vector<RawSock*>& clis);
};

#endif // TCPJPGMPARTCAM_H
