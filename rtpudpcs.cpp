#include <iostream>
#include "rtpudpcs.h"


#define OUT_STREAM  nullout

rtpudpcs::rtpudpcs(int port):_port(port),_frmcount(0),_ok(false)
{
    _udp.destroy();
    _udpc.destroy();
    if(!_udp.create(_port))
    {
         return;
    }
    if(!_udp.bind(0,_port))
    {
         return;
    }
    if(!_udpc.create(_port+1))
    {
         return;
    }
    if(!_udpc.bind(0,_port+1))
    {
         return;
    }
    _frame = new uint8_t[_frame_len];
    _ok=true;
}

rtpudpcs::~rtpudpcs()
{
    _udp.destroy();
    _udpc.destroy();
    delete[] _frame;
}


bool rtpudpcs::ok()const
{
    return _ok;
}

int rtpudpcs::spin(event_t&)
{
    int         rv;
    int         bytes1 = 0;
    int         bytes2 = 0;
    rv  = _pool(_udp,_udpc);
    if(rv<0)
        return 0;
    if(rv==0) //closed remote not reached
    {
        ::usleep(0xFFF);
        return 0;
    }
    if(rv&1)
    {
        // video port
        bytes1 = _udp.receive(_frame,MAX_BUFF);
        if(bytes1){
            printf("git frmo UDP1 %d bytes\n", bytes1);
        }
    }
    if(rv&2)
    {
        bytes2 = _udpc.receive(_frame+bytes1,MAX_BUFF-bytes1);
        if(bytes2){
            printf("git frmo UDP2 %d bytes\n", bytes2);
    }
    }
/*
    char oct[8];
    TRACE()<<"\n";
    for(int i=0; i < 128; i++)
    {
        if(_frame[i]>=' '&&_frame[i]<='z')
            sprintf(oct,"%c ", (int)_frame[i]);
        else
            sprintf(oct,"%c ", (int)'.');
        TRACE()<<oct;
        if(((i+1)%16)==0)TRACE()<<"\n";
    }
    TRACE()<<"\n";

    for(int i=0; i < 128; i++)
    {
        sprintf(oct,"%02X ", (int)_frame[i]);
        TRACE()<<oct;
        if(((i+1)%16)==0)TRACE()<<"\n";
    }
    TRACE()<<"\n";
*/
    return bytes1+bytes2;
}

int rtpudpcs::_pool(udp_sock& s, udp_sock& c )
{
    struct timeval  tv;
    int             ret=0;
    fd_set          r;
    const SOCKET    nsock = s.socket();
    const SOCKET    nsock1 = c.socket();
    const int       ndfs = std::max(nsock,nsock1)+1;

    FD_SET(nsock, &r);
    FD_SET(nsock1, &r);
    tv.tv_sec = 0;
    tv.tv_usec = 8000;

    ret = select(ndfs, &r, 0, 0, &tv);
    if(ret<0)
    {
        return ret;
    }
    if(ret > 0)
    {
        ret = 0;
        ret |= FD_ISSET(nsock, &r) ? 0x1  : 0;
        ret |= FD_ISSET(nsock1, &r) ? 0x2  : 0;
    }
    return ret;
}
