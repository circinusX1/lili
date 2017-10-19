///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TCPLIBAV_H
#define TCPLIBAV_H

#include "tcpcamcli.h"



class TcpLibAvi : public TcpCamCli
{
public:
    TcpLibAvi(RawSock& ,
              const LiFrmHdr& h,const char* pextra);
};

#endif // TCPLIBAV_H
