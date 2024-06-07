///////////////////////////////////////////////////////////////////////////////////////////////
#include "main.h"
#include "rawsock.h"
#include "pool.h"

/**
 * @brief RawSock::RawSock
 * @param r
 * @param h
 * @param t
 */
RawSock::RawSock(RawSock& r,
                   const LiFrmHdr& h,RawSock::STYPE t):
                                        tcp_cli_sock(r),
                                        _t(t)
{
    r.detach();
    _header = h;
    _name = _header.camname;
    _mac = _header.mac;
    _data.cap=MAX_BUFF-1;
    _data.buff=new uint8_t[MAX_BUFF];
    _data.reset();
}

/**
 * @brief RawSock::~RawSock
 */
RawSock::~RawSock()
{
    delete[] _data.buff;
}

/**
 * @brief RawSock::recdata
 * @return
 */
int RawSock::recdata()
{
    int bytes = this->receive(_data.rec_ptr(),_data.room());
    if(bytes<=0 || _letal)
    {
        GLOGE("rec data =0" << errno );
        this->destroy();
        throw _t;
    }
    _data.inbytes += bytes;
    _data.rec_off += bytes;
    return (_data.rec_off - _data.prc_off);
}

int RawSock::recdata(uint8_t* buff,size_t rec_off)
{
    int bytes = 0;
    if(rec_off){
        bytes = this->receive(buff,rec_off);
        if(bytes==0){
            this->destroy();
            throw _t;
        }
    }
    if(bytes>0)
    {
        buff[bytes]=0;
    }
    return bytes;
}

/**
** * @brief RawSock::can_send
** */
void RawSock::can_send(bool force)
{
}


/**
 * @brief RawSock::snd
 * @param b
 * @param room
 * @param extra
 * @return
 */
int RawSock::snd(const uint8_t* b,size_t room,
                  uint32_t extra)
{
    int rb = this->sendall(b,room);
    if((size_t)rb!=room){
        this->destroy();
        throw _t;
    }
    return rb;
}

/**
 * @brief RawSock::destroy
 * @param be
 */
bool RawSock::destroy(bool be)
{
    return tcp_cli_sock::destroy(be);
}

/**
 * @brief RawSock::isopen
 * @return
 */
bool    RawSock::isopen()
{
    return tcp_cli_sock::isopen();
}

/**
 * @brief RawSock::transfer
 * @param to
 * @return
 */
int RawSock::transfer(const std::vector<RawSock*>& clis)
{
    int bytes = 0;
    bytes = this->recdata();
    if(clis.size())
    {
        for(const auto& css : clis)
        {
            css->snd(_data.buff,_data.rec_off,0);
        }
    }
    _data.reset();
    return bytes;
}

