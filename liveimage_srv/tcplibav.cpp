///////////////////////////////////////////////////////////////////////////////////////////////
#include "tcplibav.h"

TcpLibAvi::TcpLibAvi(RawSock& rs,
                     const LiFrmHdr& h,const char* pextra):
                     TcpCamCli(rs,h,pextra)
{

}
