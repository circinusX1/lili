#ifndef RTPUDPCS_H
#define RTPUDPCS_H

#include "os.h"
#include "sock.h"
#include "lilitypes.h"

#define MAX_BUFF   (1024*4)

class rtpudpcs
{
public:
    rtpudpcs(int port);
    virtual ~rtpudpcs();
    bool ok()const;
    int spin(event_t& event);
    const uint8_t* frame()const{ return _frame;}

protected:
    int _pool(udp_sock& s, udp_sock& c);

private:

    int          _port=0;
    uint32_t     _frmcount=0;
    udp_sock     _udp;
    udp_sock     _udpc;
    size_t       _ok;
    uint8_t*     _frame = nullptr;
    size_t       _frame_len = MAX_BUFF;
};

#endif // RTPUDPCS_H
