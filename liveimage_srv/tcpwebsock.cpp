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
int TcpWebSock::snd(const uint8_t* b,size_t room,uint32_t extra,
                 const char* oth)
{
    int sent = 1;

    _nowt = SECS();
    if(!_headered && oth && *oth){
        sent = RawSock::snd((const uint8_t*)oth,::strlen(oth),0,nullptr);
        _headered=true;
    }
    if(sent){
        return RawSock::snd(b,room,extra,nullptr);
    }
    this->destroy();
    throw _t;
    return 0;
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
