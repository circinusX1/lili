///////////////////////////////////////////////////////////////////////////////////////////////
#include "tcpwebsock.h"
#include "logger.h"

/**
 * @brief TcpWebSock::TcpWebSock
 * @param o
 * @param h
 * @param st
 */
TcpWebSock::TcpWebSock(RawSock& o,
                 const LiFrmHdr& h,
                 STYPE st):RawSock(o,h,st)
{
    GLOGD(__FUNCTION__);
    _headered = false;
}

/**
 * @brief TcpWebSock::~TcpWebSock
 */
TcpWebSock::~TcpWebSock()
{
    GLOGD(__FUNCTION__);
}


/**
 * @brief TcpWebSock::snd
 * @param b
 * @param room
 * @param extra
 * @return
 */
int TcpWebSock::snd(const uint8_t* b,size_t room,uint32_t extra)
{
    return RawSock::snd(b,room,extra);
}

/**
 * @brief TcpWebSock::destroy
 * @param be
 * @return
 */
bool TcpWebSock::destroy(bool be)
{
    _headered=false;
    return RawSock::destroy(be);
}
